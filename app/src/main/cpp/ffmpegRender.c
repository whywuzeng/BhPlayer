//
// Created by Administrator on 2018-12-10.
//

#include <jni.h>
#include "com_palyer_wz1_bhplayer_VideoPlayer.h"
#include "include/libavformat/avformat.h"
#include "include/libavcodec/avcodec.h"
#include "include/libavutil/avutil.h"
#include "include/libavutil/frame.h"
#include "include/libavutil/imgutils.h"
#include "include/libswscale/swscale.h"
#include <android/native_window_jni.h>
#include <android/native_window.h>
#include <android/log.h>

#include <stdio.h>
#include <unistd.h>

#define LOG_TAG "ffmpegAndroidPlayer"
#define LOG_INF(FORMAT,...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,FORMAT,__VA_ARGS__);
#define LOG_DEB(FORMAT,...)  __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,FORMAT,__VA_ARGS__);
#define LOG_ERR(FORMAT,...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,FORMAT,__VA_ARGS__);


JNIEXPORT void JNICALL Java_com_palyer_wz1_bhplayer_VideoPlayer_render
(JNIEnv *env, jobject jobject1, jstring jstring , jobject jobject2){
    LOG_ERR("输出error%d",1111);
    const char *inputPath = (*env)->GetStringUTFChars(env, jstring, NULL);

    av_register_all();

    AVFormatContext *pContext = avformat_alloc_context();

    if (avformat_open_input(&pContext,inputPath,NULL,NULL)!=0)
    {
        LOG_ERR("打开文件失败 %s",inputPath);
        return;
    }

    if (avformat_find_stream_info(pContext,NULL)<0)
    {
        LOG_ERR("Couldn't find stream info:%s","");
        return;
    }

    int videoStream =-1,i;
    for (int i = 0; i < pContext->nb_streams; ++i) {
        if (pContext->streams[i]->codec->coder_type ==AVMEDIA_TYPE_VIDEO&&videoStream < 0){
            videoStream=i;
        }
    }

    if (videoStream ==-1)
    {
        LOG_ERR("Didn't find video stream %s","" );
        return;
    }

    struct AVCodecContext *avCodecContext= pContext->streams[videoStream]->codec;

    enum AVCodecID id = pContext->streams[videoStream]->codec->codec_id;

    AVCodec *pCodec = avcodec_find_decoder(id);

    if (pCodec ==NULL)
    {
        LOG_ERR("pCodec is NULL %s","");
        return;
    }

    if (avcodec_open2(avCodecContext,pCodec,NULL)<0)
    {
        LOG_ERR("Could not open codec.%s","")
        return;
    }

    ANativeWindow *pWindow = ANativeWindow_fromSurface(env, jobject2);

    int width = avCodecContext->width;
    int height = avCodecContext->height;

    // 设置native window的buffer大小,可自动拉伸
    ANativeWindow_setBuffersGeometry(pWindow, width, height,
                                     WINDOW_FORMAT_RGBA_8888);

    ANativeWindow_Buffer windowBuffer;

    if (avcodec_open2(avCodecContext,pCodec,NULL)<0)
    {
        LOG_ERR("Could not open codec%s","")
        return;
    }

    AVFrame *pFrame = av_frame_alloc();

    AVFrame *RGBFrame = av_frame_alloc();
    if (pFrame==NULL || RGBFrame==NULL)
    {
        LOG_ERR("Could not alloc frame%s","")
        return;
    }

    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGBA, width, height, 1);

   uint8_t *buffer= av_malloc(numBytes* sizeof(uint8_t));

    av_image_fill_arrays(RGBFrame->data,RGBFrame->linesize,buffer,AV_PIX_FMT_RGBA,avCodecContext->width,avCodecContext->height,1);


    struct SwsContext *const pSwsContext = sws_getContext(avCodecContext->width, avCodecContext->height, avCodecContext->get_format
                , avCodecContext->width
                , avCodecContext->height
                , AV_PIX_FMT_RGBA
                , SWS_BILINEAR
                , NULL
                , NULL
                , NULL);

    int frameFinished;
    AVPacket packet;

    while (av_read_frame(pContext,&packet)>=0)
    {
        if (packet.stream_index==videoStream)
        {
            avcodec_decode_video2(avCodecContext,pFrame,&frameFinished,packet);

            if (frameFinished){
                ANativeWindow_lock(pWindow,&windowBuffer,0);
                sws_scale(pSwsContext,(uint8_t const *const *)pFrame->data,pFrame->linesize,0,avCodecContext->height,RGBFrame->data,RGBFrame->linesize);
                // 获取stride
                uint8_t *dst = (uint8_t *) windowBuffer.bits;
                int dstStride = windowBuffer.stride * 4;
                uint8_t *src = (RGBFrame->data[0]);
                int srcStride = RGBFrame->linesize[0];

                // 由于window的stride和帧的stride不同,因此需要逐行复制
                int h;
                for (h = 0; h < height; h++) {
                    memcpy(dst + h * dstStride, src + h * srcStride, srcStride);
                }

                ANativeWindow_unlockAndPost(pWindow);
                usleep(1000*16);
            }
        }

        av_packet_unref(&packet);
    }

    free(buffer);
    av_frame_free(&RGBFrame);
    av_frame_free(&pFrame);
    avcodec_close(avCodecContext);

    avformat_close_input(&pContext);

    (*env)->ReleaseStringUTFChars(env,jstring,inputPath);
    return;
}