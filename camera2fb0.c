#include <unistd.h>
#include <linux/fb.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <getopt.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <asm/types.h>
#include <linux/videodev2.h>

#define CAMERA_DEVICE "/dev/video0"
#include "SDL2/SDL.h"

#define VIDEO_WIDTH 1280
#define VIDEO_HEIGHT 720

int fd;

#define BUFFER_COUNT 4

typedef struct VideoBuffer {
	void *start;
	size_t length;
} VideoBuffer;

VideoBuffer framebuf[BUFFER_COUNT];

int check_device_capability()
{
	struct v4l2_capability cap;
	if (ioctl(fd, VIDIOC_QUERYCAP, &cap) < 0) {
		printf("VIDIOC_QUERYCAP failed\n");
		return -1;
	}
	if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
		printf(" is not an video capture device\n");
		return -1;
	}
	if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
		printf("does not support streaming i/o\n");
		return -1;
	}

	return 0;
}

#define VIDIOC_S_POWER  (_IOWR('V', 103, int))

int camera_private_cmd()
{

	int power = 1;
	if (ioctl(fd, VIDIOC_S_POWER, &power) < 0) {
		printf("VIDIOC_S_POWER failed\n");
		//close(fd);
		//return;
	}

	int input = 1;
	if (ioctl(fd, VIDIOC_S_INPUT, &input) < 0) {
		printf("VIDIOC_S_INPUT failed\n");
		//close(fd);
		//return;
	}

	return 0;
}

void set_cropcap_para()
{
	struct v4l2_cropcap cropcap;
	memset(&cropcap, 0, sizeof(cropcap));
	cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (ioctl(fd, VIDIOC_CROPCAP, &cropcap) < 0) {
		printf("VIDIOC_CROPCAP return < 0");
		struct v4l2_crop crop;
		crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		crop.c = cropcap.defrect;	/* reset to default */

		if (ioctl(fd, VIDIOC_S_CROP, &crop) < 0) {
			printf("VIDIOC_S_CROP FAILED\n");
		}
	}
}

void set_stream_parm()
{
	struct v4l2_streamparm parm;
	parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	parm.parm.capture.timeperframe.numerator = 1;
	parm.parm.capture.timeperframe.denominator = 0;	// may be not support set fps
	parm.parm.capture.capturemode = 0;
	if (ioctl(fd, VIDIOC_S_PARM, &parm) < 0) {
		printf("VIDIOC_S_PARM failed\n");
		//close(fd);
		//return;
	}
}

int set_camera_format()
{
#if 0
	struct v4l2_fmtdesc fmtdesc;
	fmtdesc.index = 0;
	fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	printf("Support format:\n");
	while (ioctl(fd, VIDIOC_ENUM_FMT, &fmtdesc) != -1
	       && fmtdesc.index < 100) {
		printf("\t%d.%s\n", fmtdesc.index + 1, fmtdesc.description);
		fmtdesc.index++;
	}
#endif

	struct v4l2_format fmt;
	memset(&fmt, 0, sizeof(fmt));
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width = VIDEO_WIDTH;
	fmt.fmt.pix.height = VIDEO_HEIGHT;
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_GREY;
	fmt.fmt.pix.field = V4L2_FIELD_ANY;

	printf("set pixformat:0x%x, width:%d, height:%d\n",
	       fmt.fmt.pix.pixelformat, fmt.fmt.pix.width, fmt.fmt.pix.height);

	/* Note VIDIOC_S_FMT may change width and height. */
	if (ioctl(fd, VIDIOC_S_FMT, &fmt) < 0) {
		printf("VIDIOC_S_FMT FAILED\n");
		//close(fd);
		//return;
	}

	/* some camera not support set format, so we get format to ensure the width and height */
	if (ioctl(fd, VIDIOC_G_FMT, &fmt) < 0) {
		printf("VIDIOC_G_FMT failed\n");
		close(fd);
		return -1;
	}

	printf("get pixformat:0x%x, width:%d, height:%d\n",
	       fmt.fmt.pix.pixelformat, fmt.fmt.pix.width, fmt.fmt.pix.height);

	//  if (fmt.fmt.pix.width != 640)       camera_width  = fmt.fmt.pix.width;
	// if (fmt.fmt.pix.height !=  camera_height)    camera_height = fmt.fmt.pix.height;
	//if (fmt.fmt.pix.pixelformat !=  camera_pixformat) {
	//    printf ("[%s] unknown format\n", TAG);
	//   return -1;
	//}

	return 0;
}

int main(int argc, char **argv)
{
	int i, ret;
	int screen_w = 1280;
	int screen_h = 720;
	int pixel_w = 1280;
	int pixel_h = 720;

	SDL_Window *screen;
	Uint32 pixformat = 0;
	SDL_Texture *sdlTexture;
	FILE *fp = NULL;
	SDL_Renderer *sdlRenderer;
	SDL_Rect sdlRect;
	SDL_Event event;
	struct timeval tv;
	char buffer[VIDEO_WIDTH * VIDEO_HEIGHT * 3 / 2] = {0};
	memset(buffer, 0x80, sizeof(buffer));

	//FIX: If window is resize
	sdlRect.x = 0;
	sdlRect.y = 0;
	sdlRect.w = VIDEO_WIDTH;
	sdlRect.h = VIDEO_HEIGHT;

	// 打开摄像头设备

	fd = open(CAMERA_DEVICE, O_RDWR, 0);
	if (fd < 0) {
		printf("Open %s failed\n", CAMERA_DEVICE);
		return -1;
	}

	check_device_capability();
	camera_private_cmd();

	set_cropcap_para();

	set_stream_parm();

	set_camera_format();

	//请求分配内存
	struct v4l2_requestbuffers reqbuf;

	reqbuf.count = BUFFER_COUNT;
	reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	reqbuf.memory = V4L2_MEMORY_MMAP;

	ret = ioctl(fd, VIDIOC_REQBUFS, &reqbuf);

	if (ret < 0) {
		printf("VIDIOC_REQBUFS failed (%d)\n", ret);
		return ret;
	}

	VideoBuffer *buffers = calloc(reqbuf.count, sizeof(*buffers));
	struct v4l2_buffer buf;

	for (i = 0; i < reqbuf.count; i++) {
		buf.index = i;
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;

		//获取内存空间
		ret = ioctl(fd, VIDIOC_QUERYBUF, &buf);

		if (ret < 0) {
			printf("VIDIOC_QUERYBUF (%d) failed (%d)\n", i, ret);
			return ret;
		}
		//映射到用户空间
		framebuf[i].length = buf.length;
		framebuf[i].start =
		    (char *)mmap(0, buf.length, PROT_READ | PROT_WRITE,
				 MAP_SHARED, fd, buf.m.offset);

		if (framebuf[i].start == MAP_FAILED) {
			printf("mmap (%d) failed: %s\n", i, strerror(errno));
			return -1;
		}

		ret = ioctl(fd, VIDIOC_QBUF, &buf);
		if (ret < 0) {
			printf("VIDIOC_QBUF (%d) failed (%d)\n", i, ret);
			return -1;
		}

		printf("Frame buffer %d: address=0x%x, length=%d\n", i,
		       (unsigned int)framebuf[i].start, framebuf[i].length);
	}

	// 启动视频流
	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ret = ioctl(fd, VIDIOC_STREAMON, &type);
	if (ret < 0) {
		printf("VIDIOC_STREAMON failed (%d)\n", ret);
		return ret;
	}

	if (SDL_Init(SDL_INIT_VIDEO)) {
		printf("Could not initialize SDL - %s\n", SDL_GetError());
		return -1;
	}
	//SDL 2.0 Support for multiple windows
	screen =
	    SDL_CreateWindow("Simplest Video Play SDL2",
	                     SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
	                     screen_w, screen_h, SDL_WINDOW_FULLSCREEN);
	if (!screen) {
		printf("SDL: could not create window - exiting:%s\n",
		       SDL_GetError());
		return -1;
	}

	sdlRenderer = SDL_CreateRenderer(screen, -1, SDL_RENDERER_PRESENTVSYNC);

	//IYUV: Y + U + V  (3 planes)
	//YV12: Y + V + U  (3 planes)
	pixformat = SDL_PIXELFORMAT_IYUV;

	sdlTexture =
	    SDL_CreateTexture(sdlRenderer, pixformat,
	                      SDL_TEXTUREACCESS_STREAMING, pixel_w, pixel_h);

	printf("start camera testing...\n");
	//开始视频显示
	while (1) {
		// printf("aaaLine:%d\n",__LINE__);
		//内存空间出队列
		ret = ioctl(fd, VIDIOC_DQBUF, &buf);
		if (ret < 0) {
			printf("VIDIOC_DQBUF failed (%d)\n", ret);
			return ret;
		}
#if 0
		if (vinfo.bits_per_pixel == 32) {
			for (y = 0; y < vinfo.yres; y++) {
				for (x = 0; x < vinfo.xres; x++) {
					//YUV422转换为RGB32
					process_rgb32(fbp32 + y * vinfo.xres + x,
						      framebuf[buf.index].start + y * vinfo.xres * 2 + x * 2);
					x++;
					// printf("aaaLine:%d\n",__LINE__);
				}
			}
		}

		if (vinfo.bits_per_pixel == 16) {
			for (y = 0; y < vinfo.yres; y++) {
				for (x = 0; x < vinfo.xres; x++) {
					//YUV422转换为RGB565
					process_rgb16(((unsigned short *)fbp32 + y * vinfo.xres + x),
						      framebuf[buf.index].start + y * vinfo.xres + x);
					//x++;

					// printf("aaaLine:%d\n",__LINE__);
				}
			}
		}


#endif
		memcpy(buffer, framebuf[buf.index].start, VIDEO_WIDTH * VIDEO_HEIGHT);
		gettimeofday(&tv, NULL);
		printf("%d:\t%d:%d\n", __LINE__, tv.tv_sec, tv.tv_usec);
		SDL_UpdateTexture(sdlTexture, NULL, buffer, VIDEO_WIDTH);

		SDL_RenderClear(sdlRenderer);
		SDL_RenderCopy(sdlRenderer, sdlTexture, NULL, &sdlRect);
		SDL_RenderPresent(sdlRenderer);
		gettimeofday(&tv, NULL);
		printf("%d:\t%d:%d\n", __LINE__, tv.tv_sec, tv.tv_usec);

		// 内存重新入队列
		ret = ioctl(fd, VIDIOC_QBUF, &buf);
		if (ret < 0) {
			printf("VIDIOC_QBUF failed (%d)\n", ret);
			return ret;
		}
	}

	//释放资源
	for (i = 0; i < 4; i++) {
		munmap(framebuf[i].start, framebuf[i].length);
	}

	//关闭设备
	close(fd);
	return 0;
}
