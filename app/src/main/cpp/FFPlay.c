//
// Created by Administrator on 2018-12-12.
//  音视频播放c

#include <jni.h>
#include <libavformat/avformat.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <pthread.h>
#include <libavutil/imgutils.h>
#include <unistd.h>
#include "android/log.h"
#include "queue.h"

#define TAG "FFPlay.c"
#define  LOG_ERR(FORMAT, ...)   __android_log_print(ANDROID_LOG_ERROR,TAG,FORMAT,__VA_ARGS__)
#define ERR(String)  LOG_ERR("%s",String)
#define MAX_STREAM 2

typedef struct _Player Player;

struct _Player {

    JavaVM *javaVM;
    AVFormatContext *mformatContext;

    //音频索引
    int mAudioIndex;
    //视频索引
    int mVideoIndex;

    AVCodecContext *codecContexts[MAX_STREAM];

    //视频压缩上下文
    struct SwsContext *pSwsContext;

    //音频压缩上下文
    struct SwrContext *pSwrContext;

    //window
    ANativeWindow *nativeWindow;

    //输入采样格式
    int inSampleFormat;
    //输入采样率
    int inSampleRate;
    //输出采样格式
    int outSampleFormat;
    //输出采样率
    int outSampleRate;
    //声道
    uint64_t layoutChannle;

    //方法ID
    jmethodID  audiotrack_write_pID;
    //jni Audio 对象
    jobject  callAudioTrackObject;

    //queue数组
    Queue *queueData[MAX_STREAM];

    //互斥锁
    pthread_mutex_t mutex;
    //互斥条件
    pthread_cond_t cond;

    //生产者线程的ID
    pthread_t streamReadID;

    //消费者线程的ID
    pthread_t decodeStreamID[MAX_STREAM];

};

void initInputFormatContext(const char *inCharPath, Player *pPlayer);

void freeObjectMethod(Player *player);

void initCodecContext(Player *pPlayer, int index);

void prepareVideoDecode(Player *pPlayer, JNIEnv *pInterface, jobject surface);

void prepareAudioDecode(Player *pPlayer);

void jniPrepareAudio(JNIEnv *pInterface, Player *pPlayer);

void initQueue(Player *pPlayer);

void initInputFormatContext(const char *inCharPath, Player *pPlayer) {
    int ref;
    av_register_all();
    AVFormatContext *pContext = avformat_alloc_context();

    ref = avformat_open_input(&pContext, inCharPath, NULL, NULL);
    if (ref < 0) {
        ERR("打开文件失败");
        return;
    }


    ref = avformat_find_stream_info(pContext, NULL);
    if (ref < 0) {
        ERR("找不到流信息");
        return;
    }


    for (int i = 0; i < pContext->nb_streams; ++i) {
        if (pContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
            pPlayer->mAudioIndex = i;
        } else if (pContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            pPlayer->mVideoIndex = i;
        }
    }

    pPlayer->mformatContext = pContext;
}

void initCodecContext(Player *pPlayer, int index) {

    int ref;

    AVFormatContext *pContext = pPlayer->mformatContext;

    AVCodecContext *mCodecontext = pContext->streams[index]->codec;


    const enum AVCodecID id = mCodecontext->codec_id;

    AVCodec *pCodec = avcodec_find_decoder(id);

    ref = avcodec_open2(mCodecontext, pCodec, NULL);
    if (ref < 0) {
        ERR("找不到解码器");
    }

    pPlayer->codecContexts[index] = mCodecontext;
}

void prepareVideoDecode(Player *pPlayer, JNIEnv *env, jobject surface) {

    pPlayer->nativeWindow = ANativeWindow_fromSurface(env, surface);
    AVCodecContext *pContext = pPlayer->codecContexts[pPlayer->mVideoIndex];
    int videoWidth = pContext->width;
    int videoHeight = pContext->height;

   pPlayer->pSwsContext=  sws_getContext( pContext->width, pContext->height, pContext->pix_fmt,
                                          pContext->width, pContext->height, AV_PIX_FMT_RGBA,
                                                    SWS_BILINEAR, NULL, NULL, NULL);
}

void prepareAudioDecode(Player *pPlayer) {
    struct SwrContext *pContext = swr_alloc();
    AVCodecContext *pCodecContext = pPlayer->codecContexts[pPlayer->mAudioIndex];
    //输出采样率格式
    enum AVSampleFormat outFormat = AV_SAMPLE_FMT_S16;
//    输出采样率
    int outRate = 44100;
//    输入采样率格式
    enum AVSampleFormat informat = pCodecContext->sample_fmt;
    //输入采样率
    int inRate = pCodecContext->sample_rate;
//    声道
    uint64_t inChannel = pCodecContext->channel_layout;
    //生成sws_context
   swr_alloc_set_opts(pContext,inChannel,outFormat,outRate,inChannel,informat,inRate,0,NULL);

    swr_init(pContext);

    pPlayer->inSampleFormat=informat;
    pPlayer->outSampleFormat=outFormat;
    pPlayer->inSampleRate=inRate;
    pPlayer->outSampleRate=outRate;
    pPlayer->pSwrContext=pContext;
    pPlayer->layoutChannle=inChannel;
}

void jniPrepareAudio(JNIEnv *env, Player *pPlayer) {

    jclass  aClass = (*env)->FindClass(env, "com/palyer/wz1/bhplayer/AudioUtil");

    jmethodID const methodID = (*env)->GetMethodID(env, aClass, "<init>", "()V");

    jobject const audioObj = (*env)->NewObject(env, aClass, methodID);

    jmethodID const audioTrackObj1 = (*env)->GetMethodID(env, aClass, "createAudioTrack", "(II)Landroid/media/AudioTrack;");

    jobject const callAudioTrackObject = (*env)->CallObjectMethod(env, audioObj, audioTrackObj1, pPlayer->outSampleRate, pPlayer->layoutChannle);

    jclass const audioTrackClass = (*env)->GetObjectClass(env, callAudioTrackObject);

    jmethodID const objPalyID = (*env)->GetMethodID(env, audioTrackClass, "play", "()V");

    (*env)->CallVoidMethod(env,callAudioTrackObject,objPalyID);

    jmethodID const audiotrack_write_pID = (*env)->GetMethodID(env, audioTrackClass, "write", "([BII)I");

    pPlayer->audiotrack_write_pID=audiotrack_write_pID;
    pPlayer->callAudioTrackObject=callAudioTrackObject;
}

void initQueue(Player *pPlayer) {
        Queue *pQueue = createQueue();
        pPlayer->queueData[0] = pQueue;
}
//生成者线程
void *readStreamPackte(void *arg)
{
    Player *player = (Player*)arg;

    int index =0 ;
    AVPacket packet;

    LOG_ERR("文件读packet结束的错误码 :%#x", (unsigned int)&packet);
    LOG_ERR("文件读player->mformatContext结束的错误码 :%#x", (unsigned int)player->mformatContext);
    while (av_read_frame(player->mformatContext,&packet)>=0)
    {
//        int ref=av_read_frame(player->mformatContext,&packet);
//        LOG_ERR("文件读av_read_frame结束的错误码 :%d", ref);
//        if (ref<0)
//        {
//            LOG_ERR("文件读packet结束的错误码 :%d", ref);
//            break;
//        }
        LOG_ERR("readStreamPackte:%d", index++);
        LOG_ERR("packet.stream_index:%d", packet.stream_index);
        LOG_ERR("player->mVideoIndex:%d", player->mVideoIndex);
        if (packet.stream_index == player->mVideoIndex)
        {
            pthread_mutex_lock(&player->mutex);
            LOG_ERR("packet.stream_index :%d",packet.stream_index);
            Queue *pQueue = player->queueData[packet.stream_index];
            LOG_ERR("Queue *pQueue :%#x",pQueue);
//            LOG_ERR("Queue *pQueue :%d",);
            queuePush(pQueue,packet,&player->mutex,&player->cond,0);
            pthread_mutex_unlock(&player->mutex);
        }
    }

    return 0;
}

void *decodeVideo(void * arg)
{
    ERR("111111111111111");
    Player *player = (Player *)arg;
    int streamIndex = player->mVideoIndex;

    int index =0 ;
    
    Queue *pQueue = player->queueData[player->mVideoIndex];
    ERR("22222222222222");
    LOG_ERR("文件读pQueue结束的错误码 :%#x", (unsigned int)pQueue);

    AVCodecContext *pContext = player->codecContexts[player->mVideoIndex];

    int videoWidth = pContext->width;
    int videoHeight = pContext->height;

    int numBytes2 = av_image_get_buffer_size(AV_PIX_FMT_RGBA, videoWidth, videoHeight, 1);
    LOG_ERR("文件读numBytes2结束的错误码 :%d", numBytes2);
    uint8_t *buffer1 =  (uint8_t *)av_malloc(numBytes2 * sizeof(uint8_t));
//    LOG_ERR("文件读buffer1结束的错误码 :%#x", buffer1);
    while (true) {
        LOG_ERR("decodeVideo:%d", index++);
        pthread_mutex_lock(&player->mutex);

        AVPacket packet = QueuePop(pQueue, &player->mutex, &player->cond, 0);

        AVFrame *pFrame = av_frame_alloc();

        //要设置RGBAFrame 帧
        AVFrame *RGBAFrame = av_frame_alloc();

        int gotoFrame = 0;

        if (!packet.size)
        {
            ERR("packet没data");
            break;
        }

        //帧的字节数
        int buffercodec = avcodec_decode_video2(pContext, pFrame, &gotoFrame, &packet);

        if (buffercodec<=0)
        {
            break;
        }

        if (gotoFrame ) {

            ANativeWindow_Buffer windowBuffer;
            ANativeWindow_setBuffersGeometry(player->nativeWindow,videoWidth,videoHeight,WINDOW_FORMAT_RGBA_8888);
            if (ANativeWindow_lock(player->nativeWindow,&windowBuffer,NULL)==0)
            {

            }
            av_image_fill_arrays(RGBAFrame->data,RGBAFrame->linesize,buffer1,AV_PIX_FMT_RGBA,videoWidth,videoHeight,1);

            sws_scale(player->pSwsContext, (const uint8_t *const *) pFrame->data, pFrame->linesize, 0, videoHeight, RGBAFrame->data, RGBAFrame->linesize);

            uint8_t *dst = (uint8_t *)windowBuffer.bits; //surface 窗口首地址
            int32_t windowLineByte = windowBuffer.stride * 4; //RGB首地址
            uint8_t *firstRGBAbyte = RGBAFrame->data[0];
            int rgbLineByte = RGBAFrame->linesize[0];

            for (int i = 0; i < videoHeight; ++i) {
                memcpy(dst+i*windowLineByte,firstRGBAbyte+rgbLineByte*i,rgbLineByte);
            }

            ANativeWindow_unlockAndPost(player->nativeWindow);
        }


        av_frame_free(&pFrame);
        av_frame_free(&RGBAFrame);

        pthread_mutex_unlock(&player->mutex);
    }

    return 0;
}

void freeObjectMethod(Player *player) {
    avformat_free_context(player->mformatContext);
}

JNIEXPORT  void JNICALL Java_com_palyer_wz1_bhplayer_VideoPlayer_ccPlayStream
        (JNIEnv *env, jobject jobject1, jstring js, jobject surface) {

    const char *inCharPath = (*env)->GetStringUTFChars(env, js, NULL);

    //分配Player结构体
    Player *player = (Player *) malloc(sizeof(Player *));
    initInputFormatContext(inCharPath, player);

    //视频索引
    //音频索引

    //获取音频视频解码器 打开
    const int audioIndex = player->mAudioIndex;
    const int videoIndex = player->mVideoIndex;
    //封装格式上下文
//    initCodecContext(player, audioIndex);
    initCodecContext(player, videoIndex);

    //准备视频解码
//    prepareVideoDecode(player, env, surface);
//    //准备音频解码
//    prepareAudioDecode(player);


    //jni 音频准备
//    jniPrepareAudio(env, player);
    //生成队列
    initQueue(player);
    //线程读流
    pthread_mutex_init(&player->mutex,NULL);
    pthread_cond_init(&player->cond,NULL);

    pthread_create(&(player->streamReadID), NULL, (void *) readStreamPackte, (void *) player);

    pthread_t tid_1 = player->decodeStreamID[player->mVideoIndex];
    LOG_ERR("1111111111---------------%#x",(unsigned int)&tid_1);
    sleep(1);
//    ERR("11111111111--111111111");
    //消费者线程解码

    pthread_create(&(player->decodeStreamID[player->mVideoIndex]),NULL,(void *)decodeVideo,(void *)player);

    pthread_join((player->streamReadID),NULL);
    pthread_join(player->decodeStreamID[player->mVideoIndex],NULL);

    for (int i = 0; i < MAX_STREAM; ++i) {
        freeQueue(player->queueData[i]);
        avcodec_close(player->codecContexts[i]);
    }

    pthread_cond_destroy(&player->cond);
    pthread_mutex_destroy(&player->mutex);
    //释放方法
    freeObjectMethod(player);
    free(player);
    (*env)->ReleaseStringUTFChars(env,js,inCharPath);
    return;
}
