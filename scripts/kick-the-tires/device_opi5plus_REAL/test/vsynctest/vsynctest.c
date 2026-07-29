#include <stdio.h>
#include <xf86drm.h>
#include <fcntl.h>
#include <string.h>
#include <sys/time.h>
 
#define DRM_VBLANK_HIGH_CRTC_SHIFT 1
 
struct timeval tp1, tp2;
 
void onVsync(){
    long usec1 = 0;
    gettimeofday(&tp2, NULL);
    usec1 = 1000000 * (tp2.tv_sec - tp1.tv_sec) + (tp2.tv_usec - tp1.tv_usec);
    printf("onVsync= %f ms \n",usec1/1000.0f);
    gettimeofday(&tp1, NULL);
}
 
int main(int argc, char** argv) 
{
    int ret = 0;
    drmVBlank vbl;
    
    memset(&vbl, 0, sizeof(vbl));
    vbl.request.type = (drmVBlankSeqType)(DRM_VBLANK_RELATIVE);;
    vbl.request.sequence = 0;
    
    //open card0 device point
    int fd = open("/dev/dri/card0",O_RDWR);
    if ( fd<0 ) {
        printf("failed to open /dev/dri/card0");
        return -1;
    }
 
    gettimeofday(&tp1, NULL);
    while (1) {
        uint32_t high_crtc = (0 << DRM_VBLANK_HIGH_CRTC_SHIFT);
    	vbl.request.type = (drmVBlankSeqType)(DRM_VBLANK_RELATIVE | (high_crtc & DRM_VBLANK_HIGH_CRTC_MASK) );
    	vbl.request.sequence = 1;
    	//wait next vsync
    	ret = drmWaitVBlank(fd, &vbl);
    	if (ret != 0) {
    	    printf("drmWaitVBlank failed ret=%d\n", ret);
    	    return -1;
    	}
        //vsync callback
        onVsync();
    }
    return 0;
}
