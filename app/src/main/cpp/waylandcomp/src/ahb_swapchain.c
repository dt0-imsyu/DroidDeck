#define _GNU_SOURCE
#include "ahb_swapchain.h"
#include "droiddeck_color.h"
#include "droiddeck_ext.h"
#include "sc_layer.h"
#include "vk_present.h"
#include "banner-ahb-v1-server-protocol.h"
#include <android/hardware_buffer.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <unistd.h>

#define MOD_QCOM_COMPRESSED 0x0500000000000001ULL

struct droiddeck_dma_buf_sync_file { uint32_t flags; int32_t fd; };
#define DROIDDECK_DMA_BUF_SYNC_READ 1u
#define DROIDDECK_DMA_BUF_IOCTL_EXPORT_SYNC_FILE _IOWR('b', 2, struct droiddeck_dma_buf_sync_file)
#define DROIDDECK_DMA_BUF_IOCTL_IMPORT_SYNC_FILE _IOW('b', 3, struct droiddeck_dma_buf_sync_file)

extern volatile int g_zero_copy;
extern volatile int g_output_w,g_output_h;

struct ahb_buf {
    uint64_t id;
    struct dmabuf_buffer *b;
    AHardwareBuffer *ahb;
    int w, h;
    uint32_t stride, image_count;
    uint32_t format;
    uint64_t modifier;
    struct wl_client *client;
    struct wl_resource *resource;
    struct wl_listener resource_destroy;
    int on_layer;
    int release_pending;
    int64_t deferred_ns;
    struct surface *surface;
    struct wl_event_source *fence_src;
    int fence_fd;
    struct wl_resource *explicit_owner;
    int explicit_sync, acquire_ready, acquire_fd;
    struct wl_list link;
};

static struct wl_list g_bufs;
static struct wl_list g_clients;
static uint64_t g_next_id = 1;
static struct wl_event_loop *g_loop;
static unsigned g_stat_zero_copy;
static int g_explicit_only;

static unsigned client_mode(unsigned version, int on) {
    return on && (!g_explicit_only || version >= 3) ? 1u : 0u;
}
static int g_export_failed_logged, g_import_failed_logged;
static int g_advertised;
static int g_mode_sent = -1;
static struct surface *g_announced;
static volatile int64_t g_last_zero_copy_ns;

struct released { uint64_t id; int fd; };
static pthread_mutex_t g_rel_lock = PTHREAD_MUTEX_INITIALIZER;
static struct released *g_rel;
static int g_rel_n, g_rel_cap;
static int g_rel_pipe[2] = {-1, -1};

struct chain_seen { struct wl_client *client; int w, h; uint32_t image_count; uint64_t modifier; uint32_t format; };
static struct chain_seen g_last_chain;

static int64_t now_ns(void) {
    struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts);
    return (int64_t)ts.tv_sec * 1000000000LL + ts.tv_nsec;
}

static struct ahb_buf *find_buf(uint64_t id) {
    struct ahb_buf *ab;
    wl_list_for_each(ab, &g_bufs, link)
        if (ab->id == id) return ab;
    return NULL;
}

static void buf_free(struct ahb_buf *ab) {
    if (ab->resource) { wl_list_remove(&ab->resource_destroy.link); ab->resource = NULL; }
    if (ab->fence_src) { wl_event_source_remove(ab->fence_src); ab->fence_src = NULL; }
    if (ab->fence_fd >= 0) { close(ab->fence_fd); ab->fence_fd = -1; }
    if (ab->acquire_fd >= 0) close(ab->acquire_fd);
    wl_list_remove(&ab->link);
    *droiddeck_dmabuf_ahb_slot(ab->b) = NULL;
    AHardwareBuffer_release(ab->ahb);
    droiddeck_dmabuf_unref(ab->b);
    free(ab);
}

static void maybe_free(struct ahb_buf *ab) {
    if (!ab->resource && !ab->on_layer && !ab->fence_src) buf_free(ab);
}

static void on_buffer_resource_destroyed(struct wl_listener *l, void *data) {
    struct ahb_buf *ab = wl_container_of(l, ab, resource_destroy);
    wl_list_remove(&ab->resource_destroy.link);
    ab->resource = NULL;
    ab->release_pending = 0;
    maybe_free(ab);
}


static void send_deferred_release(struct ahb_buf *ab) {
    if (ab->release_pending && ab->resource)
        droiddeck_release_buffer(ab->surface, ab->resource, ab->surface != NULL, ab->deferred_ns);
    ab->release_pending = 0;
    ab->surface = NULL;
}

static int on_fence_readable(int fd, uint32_t mask, void *data) {
    struct ahb_buf *ab = data;
    wl_event_source_remove(ab->fence_src);
    ab->fence_src = NULL;
    close(ab->fence_fd);
    ab->fence_fd = -1;
    send_deferred_release(ab);
    maybe_free(ab);
    return 0;
}

static void handle_released(uint64_t id, int fd) {
    struct ahb_buf *ab = find_buf(id);
    if (!ab) { if (fd >= 0) close(fd); return; }
    ab->on_layer = 0;
    if (ab->explicit_sync && ab->explicit_owner && ab->resource) {
        if (fd >= 0) {
            banner_ahb_v1_send_fenced_release(ab->explicit_owner, ab->resource, fd);
            close(fd);
        } else banner_ahb_v1_send_immediate_release(ab->explicit_owner, ab->resource);
        send_deferred_release(ab);
        maybe_free(ab);
        return;
    }
    if (fd >= 0) {
        struct droiddeck_dma_buf_sync_file imp = {.flags = DROIDDECK_DMA_BUF_SYNC_READ, .fd = fd};
        int dmabuf_fd = droiddeck_dmabuf_fd(ab->b);
        if (dmabuf_fd >= 0 && ioctl(dmabuf_fd, DROIDDECK_DMA_BUF_IOCTL_IMPORT_SYNC_FILE, &imp) == 0) {
            close(fd);
        } else {
            if (!g_import_failed_logged) {
                g_import_failed_logged = 1;
                droiddeck_log("layer", "zero-copy: DMA_BUF_IOCTL_IMPORT_SYNC_FILE failed (%s): releases wait for the display's fence here instead",
                           strerror(errno));
            }
            struct pollfd p = {.fd = fd, .events = POLLIN};
            if (poll(&p, 1, 0) > 0) {
                close(fd);
            } else {
                ab->fence_fd = fd;
                ab->fence_src = wl_event_loop_add_fd(g_loop, fd, WL_EVENT_READABLE, on_fence_readable, ab);
                if (!ab->fence_src) { close(fd); ab->fence_fd = -1; }
                else return;
            }
        }
    }
    send_deferred_release(ab);
    maybe_free(ab);
}

static int on_release_pipe(int fd, uint32_t mask, void *data) {
    char buf[64];
    while (read(fd, buf, sizeof(buf)) > 0) {}
    for (;;) {
        struct released r;
        pthread_mutex_lock(&g_rel_lock);
        if (g_rel_n == 0) { pthread_mutex_unlock(&g_rel_lock); break; }
        r = g_rel[0];
        memmove(g_rel, g_rel + 1, (size_t)(--g_rel_n) * sizeof(*g_rel));
        pthread_mutex_unlock(&g_rel_lock);
        handle_released(r.id, r.fd);
    }
    return 0;
}

void ahb_swapchain_layer_released(void *token, int release_fd) {
    pthread_mutex_lock(&g_rel_lock);
    if (g_rel_n == g_rel_cap) {
        int cap = g_rel_cap ? g_rel_cap * 2 : 16;
        struct released *n = realloc(g_rel, (size_t)cap * sizeof(*n));
        if (!n) { pthread_mutex_unlock(&g_rel_lock); if (release_fd >= 0) close(release_fd); return; }
        g_rel = n; g_rel_cap = cap;
    }
    g_rel[g_rel_n++] = (struct released){(uint64_t)(uintptr_t)token, release_fd};
    pthread_mutex_unlock(&g_rel_lock);
    if (g_rel_pipe[1] >= 0) { char c = 1; if (write(g_rel_pipe[1], &c, 1) < 0) {} }
}


int ahb_swapchain_has_ahb(const struct dmabuf_buffer *b) {
    return b && *droiddeck_dmabuf_ahb_slot((struct dmabuf_buffer *)b) != NULL;
}

int ahb_swapchain_present(struct dmabuf_buffer *b, struct surface *s, int scene_w, int scene_h) {
    struct ahb_buf *ab = b ? *droiddeck_dmabuf_ahb_slot(b) : NULL;
    if (!ab) return -1;
    int dmabuf_fd = droiddeck_dmabuf_fd(b);
    int acquire = -1;
    if (ab->explicit_sync) {
        if (ab->on_layer) return 0;
        if (!ab->acquire_ready) return -1;
        acquire = ab->acquire_fd;
        ab->acquire_fd = -1;
        ab->acquire_ready = 0;
    } else {
    if (dmabuf_fd < 0) return -1;
    struct droiddeck_dma_buf_sync_file exp = {.flags = DROIDDECK_DMA_BUF_SYNC_READ, .fd = -1};
    if (dmabuf_fd >= 0 && ioctl(dmabuf_fd, DROIDDECK_DMA_BUF_IOCTL_EXPORT_SYNC_FILE, &exp) == 0 && exp.fd >= 0) {
        acquire = exp.fd;
    } else if (dmabuf_fd >= 0) {
        if (!g_export_failed_logged) {
            g_export_failed_logged = 1;
            droiddeck_log("layer", "zero-copy: DMA_BUF_IOCTL_EXPORT_SYNC_FILE failed (%s): waiting for each frame on the CPU instead",
                       strerror(errno));
        }
        struct pollfd p = {.fd = dmabuf_fd, .events = POLLIN};
        int r;
        do { r = poll(&p, 1, 100); } while (r < 0 && errno == EINTR);
        if (r <= 0 || !(p.revents & POLLIN)) return -1;
    }
    }
    int r = sc_layer_present_ahb(ab->ahb, ab->w, ab->h, acquire, (void *)(uintptr_t)ab->id, scene_w, scene_h,
                                 s ? droiddeck_surface_color(s) : NULL, ab->format);
    if (r < 0) return -1;
    if (r == 1) return 0;
    if (!ab->on_layer) {
        ab->on_layer = 1;
        g_stat_zero_copy++;
        g_last_zero_copy_ns = now_ns();
    }
    if (s && g_announced != s) {
        char name[160];
        g_announced = s;
        droiddeck_surface_describe(s, name, sizeof(name));
        droiddeck_log("layer", "zero-copy: presenting %s without a copy", name);
    }
    return 0;
}

int ahb_swapchain_last_frame_age_ms(void) {
    int64_t t = g_last_zero_copy_ns;
    if (!t) return -1;
    int64_t age = (now_ns() - t) / 1000000LL;
    return age > INT32_MAX ? INT32_MAX : (int)age;
}

int ahb_swapchain_defer_release(struct dmabuf_buffer *b, struct wl_resource *buffer, struct surface *s, int paced) {
    struct ahb_buf *ab = b ? *droiddeck_dmabuf_ahb_slot(b) : NULL;
    if (ab && ab->explicit_sync && !ab->on_layer && buffer && ab->resource == buffer && !ab->fence_src) {
        if (ab->acquire_fd >= 0) close(ab->acquire_fd);
        ab->acquire_fd = -1;
        ab->acquire_ready = 0;
        if (ab->explicit_owner) banner_ahb_v1_send_immediate_release(ab->explicit_owner, buffer);
        return 0;
    }
    if (!ab || !ab->on_layer || !buffer || ab->resource != buffer) return 0;
    ab->release_pending = 1;
    ab->deferred_ns = now_ns();
    ab->surface = paced ? s : NULL;
    return 1;
}

void ahb_swapchain_surface_gone(struct surface *s) {
    struct ahb_buf *ab;
    wl_list_for_each(ab, &g_bufs, link)
        if (ab->surface == s) ab->surface = NULL;
}

unsigned ahb_swapchain_stats_take(void) {
    unsigned n = g_stat_zero_copy;
    g_stat_zero_copy = 0;
    return n;
}

int ahb_swapchain_advertised(void) { return g_advertised; }

uint32_t ahb_swapchain_ahb_format(const struct dmabuf_buffer *b) {
    struct ahb_buf *ab = b ? *droiddeck_dmabuf_ahb_slot((struct dmabuf_buffer *)b) : NULL;
    return ab ? ab->format : 0;
}


static void ahb_destroy(struct wl_client *c, struct wl_resource *r) { wl_resource_destroy(r); }

static void ahb_attach_common(struct wl_client *c, struct wl_resource *r, struct wl_resource *buffer, int32_t sock,
                       uint32_t width, uint32_t height, uint32_t stride, uint32_t mod_hi, uint32_t mod_lo,
                       uint32_t image_count, int explicit_sync) {
    struct dmabuf_buffer *b = droiddeck_dmabuf_from_resource(buffer);
    uint64_t modifier = ((uint64_t)mod_hi << 32) | mod_lo;
    if (!b) {
        droiddeck_log("layer", "zero-copy: %s attached an AHardwareBuffer to a non-dma-buf wl_buffer, ignored", droiddeck_client_name(c));
        close(sock);
        return;
    }
    if (!explicit_sync && droiddeck_dmabuf_fd(b) < 0) {
        droiddeck_log("layer", "zero-copy: buffer without dma-buf requires explicit synchronization");
        close(sock);
        return;
    }
    void **slot = droiddeck_dmabuf_ahb_slot(b);
    if (*slot) { close(sock); return; }

    int socket_type=0;socklen_t socket_type_size=sizeof(socket_type);
    struct timeval timeout={.tv_sec=2};
    if (getsockopt(sock,SOL_SOCKET,SO_TYPE,&socket_type,&socket_type_size)) {
        droiddeck_log("layer","AHB socket type query failed: fd=%d errno=%d",sock,errno);close(sock);return;
    }
    if (socket_type!=SOCK_STREAM) {
        droiddeck_log("layer","AHB socket type mismatch: fd=%d type=%d",sock,socket_type);close(sock);return;
    }
    struct timeval existing={0};socklen_t timeout_size=sizeof(existing);
    int bounded=!getsockopt(sock,SOL_SOCKET,SO_RCVTIMEO,&existing,&timeout_size) &&
        timeout_size==sizeof(existing) && existing.tv_sec>=0 && existing.tv_usec>=0 && existing.tv_usec<1000000 &&
        (existing.tv_sec || existing.tv_usec) && (existing.tv_sec<2 || (existing.tv_sec==2&&!existing.tv_usec));
    /* A received socket can be readable but not setopt-able across Android domains. */
    if (!bounded && setsockopt(sock,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(timeout))) {
        droiddeck_log("layer","AHB socket has no bounded receive timeout: fd=%d errno=%d",sock,errno);close(sock);return;
    }

    struct pollfd p = {.fd = sock, .events = POLLIN};
    int pr;
    do { pr = poll(&p, 1, 1000); } while (pr < 0 && errno == EINTR);
    AHardwareBuffer *ahb = NULL;
    int rc = pr > 0 ? AHardwareBuffer_recvHandleFromUnixSocket(sock, &ahb) : -1;
    close(sock);
    if (rc != 0 || !ahb) {
        droiddeck_log("layer", "zero-copy: receiving %s's AHardwareBuffer failed (%s)", droiddeck_client_name(c),
                   pr > 0 ? "recvHandleFromUnixSocket" : "nothing on the socket");
        return;
    }
    AHardwareBuffer_Desc d;
    AHardwareBuffer_describe(ahb, &d);
    if (explicit_sync && (d.format!=AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM || d.layers!=1 ||
        !(d.usage&AHARDWAREBUFFER_USAGE_COMPOSER_OVERLAY) || (d.usage&AHARDWAREBUFFER_USAGE_PROTECTED_CONTENT))) {
        AHardwareBuffer_release(ahb);
        wl_client_post_implementation_error(c,"unsupported explicit AHB description");
        return;
    }
    int bw, bh;
    droiddeck_dmabuf_size(b, &bw, &bh);
    if ((int)d.width != bw || (int)d.height != bh || (int)width != bw || (int)height != bh) {
        droiddeck_log("layer", "zero-copy: %s's AHardwareBuffer is %ux%u but its wl_buffer %dx%d, ignored",
                   droiddeck_client_name(c), d.width, d.height, bw, bh);
        AHardwareBuffer_release(ahb);
        return;
    }

    struct ahb_buf *ab = calloc(1, sizeof(*ab));
    if (!ab) { AHardwareBuffer_release(ahb); wl_client_post_no_memory(c); return; }
    /* Explicit clients cannot fall back to a Vulkan read without an acquire wait. */
    if (explicit_sync && droiddeck_dmabuf_make_layer_only(b) != 0) {
        AHardwareBuffer_release(ahb);
        free(ab);
        wl_client_post_implementation_error(c,"explicit AHB requires a fresh Android wl_buffer");
        return;
    }
    ab->id = g_next_id++;
    ab->b = b;
    droiddeck_dmabuf_ref(b);
    ab->ahb = ahb;
    ab->w = bw; ab->h = bh;
    ab->stride = d.stride ? d.stride : stride;
    ab->image_count = image_count;
    ab->format = d.format;
    ab->modifier = modifier;
    ab->client = c;
    ab->resource = buffer;
    ab->resource_destroy.notify = on_buffer_resource_destroyed;
    wl_resource_add_destroy_listener(buffer, &ab->resource_destroy);
    ab->fence_fd = -1;
    ab->acquire_fd = -1;
    ab->explicit_sync = explicit_sync;
    ab->explicit_owner = explicit_sync ? r : NULL;
    wl_list_insert(g_bufs.prev, &ab->link);
    *slot = ab;

    if (g_last_chain.client != c || g_last_chain.w != bw || g_last_chain.h != bh ||
        g_last_chain.image_count != image_count || g_last_chain.modifier != modifier || g_last_chain.format != d.format) {
        g_last_chain = (struct chain_seen){c, bw, bh, image_count, modifier, d.format};
        droiddeck_log("layer", "zero-copy: AHB swapchain from %s (%u images, %dx%d, %s, %s, stride %u px)", droiddeck_client_name(c),
                   image_count, bw, bh, droiddeck_ahb_format_name(d.format), modifier == MOD_QCOM_COMPRESSED ? "UBWC (QCOM_COMPRESSED)"
                                       : modifier == 0 ? "linear" : "unknown modifier", ab->stride);
    }
}

static void ahb_attach(struct wl_client *c, struct wl_resource *r, struct wl_resource *buffer, int32_t sock,
                       uint32_t width, uint32_t height, uint32_t stride, uint32_t hi, uint32_t lo,
                       uint32_t count) {
    ahb_attach_common(c,r,buffer,sock,width,height,stride,hi,lo,count,0);
}

static void ahb_attach_explicit(struct wl_client *c, struct wl_resource *r, struct wl_resource *buffer,
                               int32_t sock, uint32_t w, uint32_t h, uint32_t stride, uint32_t count) {
    ahb_attach_common(c,r,buffer,sock,w,h,stride,UINT32_MAX,UINT32_MAX,count,1);
}

static void set_acquire(struct wl_client *c, struct wl_resource *r, struct wl_resource *buffer, int fd) {
    struct dmabuf_buffer *b=droiddeck_dmabuf_from_resource(buffer);
    struct ahb_buf *ab=b?*droiddeck_dmabuf_ahb_slot(b):NULL;
    if (!ab || !ab->explicit_sync || ab->explicit_owner!=r || ab->client!=c ||
        ab->on_layer || ab->release_pending || ab->fence_src || ab->acquire_ready) {
        if(fd>=0)close(fd);
        wl_client_post_implementation_error(c,"AHB acquire for invalid or busy buffer");
        return;
    }
    ab->acquire_fd=fd;ab->acquire_ready=1;
}
static void ahb_set_acquire_fence(struct wl_client *c, struct wl_resource *r, struct wl_resource *buffer,int32_t fd) {
    set_acquire(c,r,buffer,fd);
}
static void ahb_set_acquire_ready(struct wl_client *c, struct wl_resource *r, struct wl_resource *buffer) {
    set_acquire(c,r,buffer,-1);
}

static void ahb_create_buffer(struct wl_client *c,struct wl_resource *r,uint32_t id,int32_t socket,
                              uint32_t w,uint32_t h,uint32_t stride,uint32_t count) {
    struct wl_resource *buffer=droiddeck_create_ahb_layer_buffer(c,id,w,h);
    if(!buffer){close(socket);wl_client_post_implementation_error(c,"AHB buffer creation failed");return;}
    ahb_attach_explicit(c,r,buffer,socket,w,h,stride,count);
    struct dmabuf_buffer *b=droiddeck_dmabuf_from_resource(buffer);
    if(!b || !*droiddeck_dmabuf_ahb_slot(b)){
        wl_resource_destroy(buffer);
        wl_client_post_implementation_error(c,"AHB buffer import failed");
    }
}

static const struct banner_ahb_v1_interface ahb_impl = {
    .destroy = ahb_destroy,
    .attach = ahb_attach,
    .attach_explicit = ahb_attach_explicit,
    .set_acquire_fence = ahb_set_acquire_fence,
    .set_acquire_ready = ahb_set_acquire_ready,
    .create_buffer = ahb_create_buffer,
};

static void on_client_resource_destroyed(struct wl_resource *r) {
    struct ahb_buf *ab;
    wl_list_for_each(ab,&g_bufs,link) if(ab->explicit_owner==r)ab->explicit_owner=NULL;
    wl_list_remove(wl_resource_get_link(r));
}

static void bind_ahb(struct wl_client *c, void *data, uint32_t ver, uint32_t id) {
    static struct wl_client *last_named;
    if (ver > 3) ver = 3;
    struct wl_resource *r = wl_resource_create(c, &banner_ahb_v1_interface, (int)ver, id);
    if (!r) { wl_client_post_no_memory(c); return; }
    wl_resource_set_implementation(r, &ahb_impl, NULL, on_client_resource_destroyed);
    wl_list_insert(&g_clients, wl_resource_get_link(r));
    if (ver >= BANNER_AHB_V1_MODE_SINCE_VERSION) banner_ahb_v1_send_mode(r, client_mode(ver, g_zero_copy));
    if (last_named != c) {
        last_named = c;
        droiddeck_log("layer", "zero-copy: %s bound banner_ahb_v1 version %u (%s)", droiddeck_client_name(c), ver,
                   ver >= 2 ? "follows the live switch" : "version 1: decides from its launch environment only");
    }
}

void ahb_swapchain_set_mode(int on, int live) {
    on = on ? 1 : 0;
    g_zero_copy = on;
    if (!g_advertised) {
        if (live) droiddeck_log("layer", "zero-copy switched %s from the drawer, but no display layer is available on this device: no change",
                             on ? "on" : "off");
        return;
    }
    if (g_mode_sent == on) return;
    g_mode_sent = on;
    int told = 0;
    struct wl_resource *r;
    wl_resource_for_each(r, &g_clients) {
        if (wl_resource_get_version(r) >= BANNER_AHB_V1_MODE_SINCE_VERSION) {
            banner_ahb_v1_send_mode(r, client_mode((unsigned)wl_resource_get_version(r), on));
            told++;
        }
    }
    memset(&g_last_chain, 0, sizeof(g_last_chain));
    g_announced = NULL;
    if (live)
        droiddeck_log("layer", "zero-copy switched %s from the drawer: %d bound program%s told to rebuild their swapchains%s",
                   on ? "on" : "off", told, told == 1 ? "" : "s",
                   on ? "; frames go on the display layer once the new gralloc swapchain is up"
                      : "; gralloc frames still in flight stay on the layer (or take the copy path) until then");
    else
        droiddeck_log("layer", "zero-copy: %s at launch (%s)", on ? "on" : "off",
                   on ? "DROIDDECK_WAYLAND_ZERO_COPY=1: games present their own gralloc buffers on the display layer"
                      : "the drawer's Zero-copy presentation switch turns it on live");
    if (live) wl_display_flush_clients(droiddeck_get_display());
    droiddeck_request_redraw();
}

void ahb_swapchain_init(struct wl_display *display) {
    wl_list_init(&g_bufs);
    wl_list_init(&g_clients);
    if (!sc_layer_available()) {
        droiddeck_log("layer", "zero-copy: no display layer on this device (see the line above): banner_ahb_v1 not advertised");
        return;
    }
    g_loop = wl_display_get_event_loop(display);
    if (pipe2(g_rel_pipe, O_CLOEXEC | O_NONBLOCK) != 0) {
        droiddeck_log("error", "zero-copy: release pipe creation failed (%s); zero-copy presents disabled", strerror(errno));
        g_rel_pipe[0] = g_rel_pipe[1] = -1;
        return;
    }
    wl_event_loop_add_fd(g_loop, g_rel_pipe[0], WL_EVENT_READABLE, on_release_pipe, NULL);
    const char *experimental = getenv("DROIDDECK_EXPLICIT_AHB");
    unsigned version = experimental && !strcmp(experimental,"1") ? 3u : 2u;
    const char *runtime = getenv("XDG_RUNTIME_DIR");
    if (runtime && *runtime) {
        char path[512];struct stat marker;
        int length=snprintf(path,sizeof(path),"%s/mavk-explicit-ahb.enable",runtime);
        if(length>0&&(size_t)length<sizeof(path)&&!lstat(path,&marker)&&S_ISREG(marker.st_mode)&&marker.st_uid==getuid()) {
            version=3;g_zero_copy=1;g_explicit_only=1;
            if(marker.st_size<=64){
                FILE *config=fopen(path,"r");unsigned w=0,h=0;
                if(config){
                    /* Any display size the session's own marker names: the AHB window must be
                       able to match the session scene (e.g. Xwayland fullscreen 2560x1600). */
                    if(fscanf(config,"%u %u",&w,&h)==2&&w>=320&&h>=240&&w<=4096&&h<=4096&&w%2==0&&h%2==0){g_output_w=(int)w;g_output_h=(int)h;}
                    fclose(config);
                }
            }
        }
    }
    if (!wl_global_create(display, &banner_ahb_v1_interface, version, NULL, bind_ahb)) {
        droiddeck_log("error", "zero-copy: banner_ahb_v1 global creation failed");
        return;
    }
    g_advertised = 1;
    droiddeck_log("layer", "zero-copy: banner_ahb_v1 version %u advertised",version);
    ahb_swapchain_set_mode(g_zero_copy, 0);
}
