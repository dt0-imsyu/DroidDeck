#define _GNU_SOURCE
#include <assert.h>
#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <android/hardware_buffer.h>
#include <sys/socket.h>
#include <unistd.h>
#include <wayland-server.h>

struct dmabuf_buffer;
static void protocol_check(void *module,unsigned version){
    void (*init)(struct wl_display*)=dlsym(module,"ahb_swapchain_init");assert(init);
    struct wl_display *display=wl_display_create();assert(display);
    if(version==3)setenv("DROIDDECK_EXPLICIT_AHB","1",1);else unsetenv("DROIDDECK_EXPLICIT_AHB");
    init(display);
    int pair[2];assert(!socketpair(AF_UNIX,SOCK_STREAM|SOCK_CLOEXEC,0,pair));
    struct wl_client *client=wl_client_create(display,pair[0]);assert(client);
    uint32_t registry[]={1,(12u<<16)|1u,2};assert(write(pair[1],registry,sizeof(registry))==sizeof(registry));
    wl_event_loop_dispatch(wl_display_get_event_loop(display),0);wl_client_flush(client);
    unsigned char events[1024];ssize_t bytes=recv(pair[1],events,sizeof(events),MSG_DONTWAIT);assert(bytes>0);
    uint32_t name=0;size_t offset=0;
    while(offset+8<=(size_t)bytes){
        uint32_t header[2];memcpy(header,events+offset,8);unsigned size=header[1]>>16;assert(size>=8&&offset+size<=(size_t)bytes);
        if(header[0]==2&&(header[1]&0xffffu)==0){
            uint32_t n,len,v;memcpy(&n,events+offset+8,4);memcpy(&len,events+offset+12,4);
            assert(len&&16+((len+3)&~3u)+4<=size);memcpy(&v,events+offset+16+((len+3)&~3u),4);
            if(len==strlen("banner_ahb_v1")+1&&!memcmp(events+offset+16,"banner_ahb_v1",len)){name=n;assert(v==version);}
        }
        offset+=size;
    }
    assert(name);
    if(version==3){
        uint32_t bind[16]={2,0,name,0};unsigned len=(unsigned)strlen("banner_ahb_v1")+1,padded=(len+3)&~3u;
        bind[3]=len;memcpy((char*)bind+16,"banner_ahb_v1",len);bind[4+padded/4]=3;bind[5+padded/4]=3;
        unsigned bind_bytes=24+padded;bind[1]=bind_bytes<<16;
        assert(write(pair[1],bind,bind_bytes)==(ssize_t)bind_bytes);wl_event_loop_dispatch(wl_display_get_event_loop(display),0);
        AHardwareBuffer_Desc desc={.width=64,.height=64,.layers=1,.format=1,.usage=AHARDWAREBUFFER_USAGE_GPU_COLOR_OUTPUT|AHARDWAREBUFFER_USAGE_GPU_SAMPLED_IMAGE|AHARDWAREBUFFER_USAGE_COMPOSER_OVERLAY};
        AHardwareBuffer *ahb=NULL;assert(!AHardwareBuffer_allocate(&desc,&ahb));AHardwareBuffer_describe(ahb,&desc);
        int wire[2];assert(!socketpair(AF_UNIX,SOCK_STREAM|SOCK_CLOEXEC,0,wire));assert(!AHardwareBuffer_sendHandleToUnixSocket(ahb,wire[0]));
        uint32_t create[]={3,(28u<<16)|5u,4,64,64,desc.stride,3};
        struct iovec io={.iov_base=create,.iov_len=sizeof(create)};char control[CMSG_SPACE(sizeof(int))]={0};
        struct msghdr msg={.msg_iov=&io,.msg_iovlen=1,.msg_control=control,.msg_controllen=sizeof(control)};
        struct cmsghdr *c=CMSG_FIRSTHDR(&msg);c->cmsg_level=SOL_SOCKET;c->cmsg_type=SCM_RIGHTS;c->cmsg_len=CMSG_LEN(sizeof(int));memcpy(CMSG_DATA(c),&wire[1],sizeof(int));
        assert(sendmsg(pair[1],&msg,0)==sizeof(create));close(wire[1]);close(wire[0]);
        wl_event_loop_dispatch(wl_display_get_event_loop(display),0);
        struct wl_resource *resource=wl_client_get_object(client,4);assert(resource);
        struct dmabuf_buffer *(*from)(struct wl_resource*)=dlsym(module,"droiddeck_dmabuf_from_resource");
        void **(*slot)(struct dmabuf_buffer*)=dlsym(module,"droiddeck_dmabuf_ahb_slot");assert(from&&slot);
        assert(*slot(from(resource)));
        uint32_t ready[]={3,(12u<<16)|4u,4};assert(write(pair[1],ready,sizeof(ready))==sizeof(ready));
        wl_event_loop_dispatch(wl_display_get_event_loop(display),0);assert(wl_client_get_object(client,4));
        AHardwareBuffer_release(ahb);
    }
    wl_client_destroy(client);close(pair[1]);wl_display_destroy(display);
    printf("AHB_PROTOCOL_WIRE_PASS advertised=%u realWaylandDispatch=1 realAhbImport=%u NO_SURFACE_TEST=1\n",version,version==3);
}
int main(int argc,char **argv){
    assert(argc==2||argc==3);
    void *module=dlopen(argv[1],RTLD_NOW|RTLD_LOCAL);
    if(!module){fprintf(stderr,"candidate load failed: %s\n",dlerror());return 2;}
    struct wl_resource *(*create)(struct wl_client*,uint32_t,uint32_t,uint32_t)=dlsym(module,"droiddeck_create_ahb_layer_buffer");
    struct dmabuf_buffer *(*from)(struct wl_resource*)=dlsym(module,"droiddeck_dmabuf_from_resource");
    int (*fd)(const struct dmabuf_buffer*)=dlsym(module,"droiddeck_dmabuf_fd");
    int (*layer_only)(struct dmabuf_buffer*)=dlsym(module,"droiddeck_dmabuf_make_layer_only");
    void (*size)(const struct dmabuf_buffer*,int*,int*)=dlsym(module,"droiddeck_dmabuf_size");
    void (*ref)(struct dmabuf_buffer*)=dlsym(module,"droiddeck_dmabuf_ref");
    void (*unref)(struct dmabuf_buffer*)=dlsym(module,"droiddeck_dmabuf_unref");
    assert(create&&from&&fd&&layer_only&&size&&ref&&unref);
    struct wl_display *display=wl_display_create();assert(display);
    int pair[2];assert(!socketpair(AF_UNIX,SOCK_STREAM|SOCK_CLOEXEC,0,pair));
    struct wl_client *client=wl_client_create(display,pair[0]);assert(client);
    assert(!create(client,2,0,64)&&!create(client,2,64,9000));
    struct wl_resource *resource=create(client,2,1920,1200);assert(resource);
    struct dmabuf_buffer *buffer=from(resource);assert(buffer&&fd(buffer)==-1);
    int w=0,h=0;size(buffer,&w,&h);assert(w==1920&&h==1200);
    assert(layer_only(buffer)==0);
    ref(buffer);assert(layer_only(buffer)==-1);unref(buffer);
    wl_resource_destroy(resource);wl_client_destroy(client);close(pair[1]);wl_display_destroy(display);
    puts("AHB_LAYER_BUFFER_FACTORY_PASS actualProductModule=1 noDmaBufFd=1 bounds=1 freshOnly=1 resourceDestroy=1 NO_SURFACE_TEST=1");
    protocol_check(module,argc==3?3u:2u);
    dlclose(module);
    return 0;
}
