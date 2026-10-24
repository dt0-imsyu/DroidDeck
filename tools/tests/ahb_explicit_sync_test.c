#define _GNU_SOURCE
#include <assert.h>
#include <android/hardware_buffer.h>
#include <sys/eventfd.h>
#include <sys/socket.h>
#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>

struct dmabuf_buffer { int refs,fd,width,height; void *ahb_state; int layer_only; };
#include "../../app/src/main/cpp/waylandcomp/src/ahb_swapchain.c"

volatile int g_zero_copy=1;
volatile int g_output_w,g_output_h;
static unsigned presented,releases;
static int last_acquire=-2;
int sc_layer_available(void){return 1;}
int sc_layer_present_ahb(AHardwareBuffer *b,int w,int h,int fd,void *token,int sw,int sh,const struct droiddeck_color *color,uint32_t fmt){
    assert(b&&token&&w==64&&h==64&&sw==64&&sh==64&&fmt==1);
    (void)color;last_acquire=fd;presented++;if(fd>=0)close(fd);return 0;
}
void droiddeck_log(const char *tag,const char *fmt,...){(void)tag;(void)fmt;}
const char *droiddeck_client_name(struct wl_client *c){(void)c;return "test";}
void droiddeck_request_redraw(void){}
void droiddeck_surface_describe(const struct surface *s,char *out,size_t size){(void)s;if(size)out[0]=0;}
const struct droiddeck_color *droiddeck_surface_color(const struct surface *s){(void)s;return NULL;}
const char *droiddeck_ahb_format_name(uint32_t format){(void)format;return "RGBA8";}
struct dmabuf_buffer *droiddeck_dmabuf_from_resource(struct wl_resource *r){return wl_resource_get_user_data(r);}
int droiddeck_dmabuf_fd(const struct dmabuf_buffer *b){return b->fd;}
void droiddeck_dmabuf_size(const struct dmabuf_buffer *b,int *w,int *h){*w=b->width;*h=b->height;}
void **droiddeck_dmabuf_ahb_slot(struct dmabuf_buffer *b){return &b->ahb_state;}
void droiddeck_dmabuf_ref(struct dmabuf_buffer *b){b->refs++;}
void droiddeck_dmabuf_unref(struct dmabuf_buffer *b){b->refs--;}
int droiddeck_dmabuf_make_layer_only(struct dmabuf_buffer *b){if(b->refs!=1)return -1;b->layer_only=1;return 0;}
void droiddeck_release_buffer(struct surface *s,struct wl_resource *b,int paced,int64_t ns){(void)s;(void)b;(void)paced;(void)ns;releases++;}
struct wl_display *droiddeck_get_display(void){return NULL;}
struct wl_resource *droiddeck_create_ahb_layer_buffer(struct wl_client *c,uint32_t id,uint32_t w,uint32_t h){(void)c;(void)id;(void)w;(void)h;return NULL;}

int main(void){
    g_explicit_only=0;assert(client_mode(2,1)==1&&client_mode(3,1)==1);
    g_explicit_only=1;assert(client_mode(2,1)==0&&client_mode(3,1)==1&&client_mode(3,0)==0);
    g_explicit_only=0;
    struct wl_display *display=wl_display_create();assert(display);
    wl_list_init(&g_bufs);wl_list_init(&g_clients);g_loop=wl_display_get_event_loop(display);
    int peer[2];assert(!socketpair(AF_UNIX,SOCK_STREAM|SOCK_CLOEXEC,0,peer));
    struct wl_client *client=wl_client_create(display,peer[0]);assert(client);
    struct wl_resource *owner=wl_resource_create(client,&banner_ahb_v1_interface,3,2);assert(owner);
    wl_resource_set_implementation(owner,&ahb_impl,NULL,on_client_resource_destroyed);
    wl_list_insert(&g_clients,wl_resource_get_link(owner));
    struct dmabuf_buffer b={.refs=1,.fd=-1,.width=64,.height=64};
    struct wl_resource *buffer=wl_resource_create(client,&wl_buffer_interface,1,3);assert(buffer);
    wl_resource_set_user_data(buffer,&b);
    AHardwareBuffer_Desc desc={.width=64,.height=64,.layers=1,.format=1,.usage=AHARDWAREBUFFER_USAGE_GPU_COLOR_OUTPUT|AHARDWAREBUFFER_USAGE_GPU_SAMPLED_IMAGE|AHARDWAREBUFFER_USAGE_COMPOSER_OVERLAY};
    AHardwareBuffer *hardware=NULL;assert(!AHardwareBuffer_allocate(&desc,&hardware));AHardwareBuffer_describe(hardware,&desc);
    int wire[2];assert(!socketpair(AF_UNIX,SOCK_STREAM|SOCK_CLOEXEC,0,wire));
    assert(!AHardwareBuffer_sendHandleToUnixSocket(hardware,wire[0]));
    ahb_attach(client,owner,buffer,wire[1],64,64,desc.stride,0,0,3);close(wire[0]);
    assert(!b.ahb_state&&!b.layer_only);
    assert(!socketpair(AF_UNIX,SOCK_STREAM|SOCK_CLOEXEC,0,wire));
    assert(!AHardwareBuffer_sendHandleToUnixSocket(hardware,wire[0]));
    ahb_attach_explicit(client,owner,buffer,wire[1],64,64,desc.stride,3);close(wire[0]);
    struct ahb_buf *ab=b.ahb_state;assert(ab&&ab->explicit_sync&&b.layer_only&&b.refs==2);
    assert(ahb_swapchain_present(&b,NULL,64,64)==-1&&presented==0);
    int acquire=eventfd(0,EFD_CLOEXEC|EFD_NONBLOCK);assert(acquire>=0);
    ahb_set_acquire_fence(client,owner,buffer,acquire);
    assert(ahb_swapchain_present(&b,NULL,64,64)==0&&presented==1&&last_acquire==acquire&&!ab->acquire_ready);
    assert(ahb_swapchain_present(&b,NULL,64,64)==0&&presented==1);
    assert(ahb_swapchain_defer_release(&b,buffer,NULL,0)==1);
    int released=eventfd(0,EFD_CLOEXEC|EFD_NONBLOCK);assert(released>=0);
    handle_released(ab->id,released);assert(!ab->on_layer&&releases==1);
    wl_client_flush(client);
    uint32_t message[8]={0};char control[CMSG_SPACE(sizeof(int))]={0};
    struct iovec io={.iov_base=message,.iov_len=sizeof(message)};
    struct msghdr msg={.msg_iov=&io,.msg_iovlen=1,.msg_control=control,.msg_controllen=sizeof(control)};
    assert(recvmsg(peer[1],&msg,MSG_CMSG_CLOEXEC)>0);
    assert(message[0]==2&&(message[1]&0xffffu)==BANNER_AHB_V1_FENCED_RELEASE&&message[2]==3);
    struct cmsghdr *rights=CMSG_FIRSTHDR(&msg);assert(rights&&rights->cmsg_type==SCM_RIGHTS);
    int received_fd;memcpy(&received_fd,CMSG_DATA(rights),sizeof(received_fd));assert(received_fd>=0);close(received_fd);
    int dropped=eventfd(0,EFD_CLOEXEC|EFD_NONBLOCK);assert(dropped>=0);
    ahb_set_acquire_fence(client,owner,buffer,dropped);
    assert(ahb_swapchain_defer_release(&b,buffer,NULL,0)==0&&!ab->acquire_ready&&ab->acquire_fd==-1);
    assert(fcntl(dropped,F_GETFD)==-1&&errno==EBADF);
    ahb_set_acquire_ready(client,owner,buffer);
    assert(ahb_swapchain_present(&b,NULL,64,64)==0&&presented==2&&last_acquire==-1);
    uint64_t id=ab->id;
    wl_resource_destroy(owner);
    wl_resource_destroy(buffer);
    assert(b.refs==2&&find_buf(id));
    int signal=eventfd(0,EFD_CLOEXEC|EFD_NONBLOCK);assert(signal>=0);
    int release_copy=dup(signal);assert(release_copy>=0);
    handle_released(id,release_copy);assert(find_buf(id)&&b.refs==2);
    uint64_t one=1;assert(write(signal,&one,sizeof(one))==sizeof(one));
    wl_event_loop_dispatch(g_loop,0);assert(!find_buf(id)&&b.refs==1);
    close(signal);AHardwareBuffer_release(hardware);wl_client_destroy(client);close(peer[1]);wl_display_destroy(display);
    puts("AHB_EXPLICIT_STATE_PASS legacyReject=1 acquireRequired=1 acquireForwarded=1 repeatStable=1 releaseWireFd=1 skippedUseRetired=1 ownerDestroyedWait=1 bufferLifetime=1 MOCK_SURFACE_ONLY=1");
    return 0;
}
