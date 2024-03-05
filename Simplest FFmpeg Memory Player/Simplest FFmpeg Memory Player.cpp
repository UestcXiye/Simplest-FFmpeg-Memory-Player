// Simplest FFmpeg Memory Player.cpp : 定义控制台应用程序的入口点。
//

/**
* 最简单的基于 FFmpeg 的内存读写例子（内存播放器）
* Simplest FFmpeg Memory Player
*
* 源程序：
* 雷霄骅 Lei Xiaohua
* leixiaohua1020@126.com
* 中国传媒大学/数字电视技术
* Communication University of China / Digital TV Technology
* http://blog.csdn.net/leixiaohua1020
*
* 修改：
* 刘文晨 Liu Wenchen
* 812288728@qq.com
* 电子科技大学/电子信息
* University of Electronic Science and Technology of China / Electronic and Information Science
* https://blog.csdn.net/ProgramNovice
*
* 本程序实现了对内存中的视频数据的播放。
* 是最简单的使用 FFmpeg 读内存的例子。
*
* This software play video data in memory (not a file).
* It's the simplest example to use FFmpeg to read from memory.
*
*/

#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>

// 解决报错：'fopen': This function or variable may be unsafe.Consider using fopen_s instead.
#pragma warning(disable:4996)

// 解决报错：无法解析的外部符号 __imp__fprintf，该符号在函数 _ShowError 中被引用
#pragma comment(lib, "legacy_stdio_definitions.lib")
extern "C"
{
	// 解决报错：无法解析的外部符号 __imp____iob_func，该符号在函数 _ShowError 中被引用
	FILE __iob_func[3] = { *stdin, *stdout, *stderr };
}

#define __STDC_CONSTANT_MACROS

#ifdef _WIN32
// Windows
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "SDL/SDL.h"
};
#else
// Linux...
#ifdef __cplusplus
extern "C"
{
#endif
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <SDL/SDL.h>
#ifdef __cplusplus
};
#endif
#endif

// Output YUV420P 
#define OUTPUT_YUV420P 1

FILE *fp_open = NULL;

// Callback
int read_buffer(void *opaque, uint8_t *buf, int buf_size)
{
	if (!feof(fp_open))
	{
		int true_size = fread(buf, 1, buf_size, fp_open);
		return true_size;
	}
	else
	{
		return -1;
	}
}

// Refresh Event
//#define SFM_REFRESH_EVENT  (SDL_USEREVENT + 1)
//#define SFM_BREAK_EVENT  (SDL_USEREVENT + 2)
//
//int thread_exit = 0;

//int sfp_refresh_thread(void *opaque)
//{
//	thread_exit = 0;
//	while (!thread_exit)
//	{
//		SDL_Event event;
//		event.type = SFM_REFRESH_EVENT;
//		SDL_PushEvent(&event);
//		SDL_Delay(40);
//	}
//	thread_exit = 0;
//	// Break
//	SDL_Event event;
//	event.type = SFM_BREAK_EVENT;
//	SDL_PushEvent(&event);
//
//	return 0;
//}

int main(int argc, char* argv[])
{
	AVFormatContext	*pFormatCtx;
	int videoindex;
	int ret;
	AVCodecContext *pCodecCtx;
	AVCodec *pCodec;

	av_register_all();
	avformat_network_init();
	pFormatCtx = avformat_alloc_context();

	// Open File
	const char filepath[] = "cuc60anniversary_start.mkv";
	fp_open = fopen(filepath, "rb+");

	// Init AVIOContext
	unsigned char *aviobuffer = (unsigned char *)av_malloc(32768);
	AVIOContext *avio = avio_alloc_context(aviobuffer, 32768, 0, NULL, read_buffer, NULL, NULL);
	pFormatCtx->pb = avio;

	ret = avformat_open_input(&pFormatCtx, NULL, NULL, NULL);
	if (ret != 0)
	{
		printf("Couldn't open input stream.\n");
		return -1;
	}

	ret = avformat_find_stream_info(pFormatCtx, NULL);
	if (ret < 0)
	{
		printf("Couldn't find stream information.\n");
		return -1;
	}

	videoindex = -1;
	for (size_t i = 0; i < pFormatCtx->nb_streams; i++)
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			videoindex = i;
			break;
		}
	if (videoindex == -1)
	{
		printf("Couldn't find a video stream.\n");
		return -1;
	}

	pCodecCtx = pFormatCtx->streams[videoindex]->codec;
	pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
	if (pCodec == NULL)
	{
		printf("Codec not found.\n");
		return -1;
	}
	ret = avcodec_open2(pCodecCtx, pCodec, NULL);
	if (ret < 0)
	{
		printf("Could not open codec.\n");
		return -1;
	}

	AVFrame	*pFrame, *pFrameYUV;
	pFrame = av_frame_alloc();
	pFrameYUV = av_frame_alloc();

	//unsigned char *out_buffer = (unsigned char *)av_malloc(avpicture_get_size(AV_PIX_FMT_YUV420P,
	//	pCodecCtx->width, pCodecCtx->height));
	//avpicture_fill((AVPicture *)pFrameYUV, out_buffer, AV_PIX_FMT_YUV420P,
	//	pCodecCtx->width, pCodecCtx->height);

	// ------------------------ SDL 1.2 ------------------------
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER))
	{
		printf("Could not initialize SDL - %s.\n", SDL_GetError());
		return -1;
	}

	int screen_w = 0, screen_h = 0;
	SDL_Surface *screen;
	screen_w = pCodecCtx->width;
	screen_h = pCodecCtx->height;
	// 初始化屏幕（SDL 绘制的窗口）
	screen = SDL_SetVideoMode(screen_w, screen_h, 0, 0);

	if (!screen)
	{
		printf("SDL: could not set video mode - exiting:%s.\n", SDL_GetError());
		return -1;
	}
	SDL_Overlay *bmp;
	// Now we create a YUV overlay on that screen so we can input video to it
	bmp = SDL_CreateYUVOverlay(pCodecCtx->width, pCodecCtx->height, SDL_YV12_OVERLAY, screen);
	SDL_Rect rect;
	rect.x = 0;
	rect.y = 0;
	rect.w = screen_w;
	rect.h = screen_h;
	// ------------------------ SDL End ------------------------

	int got_picture;

	AVPacket *packet = (AVPacket *)av_malloc(sizeof(AVPacket));

#if OUTPUT_YUV420P 
	FILE *fp_yuv = fopen("output.yuv", "wb+");
#endif  

	struct SwsContext *img_convert_ctx;
	img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
		pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
	// SDL 线程
	//SDL_Thread *video_tid = SDL_CreateThread(sfp_refresh_thread, NULL);
	// 设置窗口标题
	SDL_WM_SetCaption("Simplest FFmpeg Memory Player", NULL);
	// Event Loop
	//SDL_Event event;

	while (av_read_frame(pFormatCtx, packet) >= 0)
	{
		if (packet->stream_index == videoindex)
		{
			ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
			if (ret < 0)
			{
				printf("Decode error.\n");
				return -1;
			}
			if (got_picture)
			{
				SDL_LockYUVOverlay(bmp);
				pFrameYUV->data[0] = bmp->pixels[0];
				pFrameYUV->data[1] = bmp->pixels[2];
				pFrameYUV->data[2] = bmp->pixels[1];
				pFrameYUV->linesize[0] = bmp->pitches[0];
				pFrameYUV->linesize[1] = bmp->pitches[2];
				pFrameYUV->linesize[2] = bmp->pitches[1];
				sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame->data,
					pFrame->linesize, 0, pCodecCtx->height, pFrameYUV->data, pFrameYUV->linesize);

#if OUTPUT_YUV420P  
				int y_size = pCodecCtx->width * pCodecCtx->height;
				fwrite(pFrameYUV->data[0], 1, y_size, fp_yuv); // Y   
				fwrite(pFrameYUV->data[1], 1, y_size / 4, fp_yuv); // U  
				fwrite(pFrameYUV->data[2], 1, y_size / 4, fp_yuv); // V  
#endif

				SDL_UnlockYUVOverlay(bmp);
				SDL_DisplayYUVOverlay(bmp, &rect);
				// Delay 40ms
				SDL_Delay(40);
			}
		}
		av_free_packet(packet);
	}
	sws_freeContext(img_convert_ctx);

#if OUTPUT_YUV420P 
	fclose(fp_yuv);
#endif 

	fclose(fp_open);
	SDL_Quit();

	// av_free(out_buffer);
	av_free(pFrameYUV);
	avcodec_close(pCodecCtx);
	avformat_close_input(&pFormatCtx);

	system("pause");
	return 0;
}

