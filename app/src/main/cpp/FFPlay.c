//
// Created by Administrator on 2018-12-12.
//  音视频播放c

#include <jni.h>
#include <libavformat/avformat.h>
#include "android/log.h"

#define TAG "FFPlay.c"
#define  LOG_ERR(FORMAT,...)   __android_log_print(ANDROID_LOG_ERROR,TAG,FORMAT,__VA_ARGS__)
#define ERR(String)  LOG_ERR("%s",String)
#define MAX_STREAM 2

typedef struct _Player Player;

struct _Player{

    JavaVM * javaVM;
    AVFormatContext *mformatContext;

    //音频索引
    int mAudioIndex;
    //视频索引
    int mVideoIndex;

    AVCodecContext *codecContexts[MAX_STREAM];

};

void initInputFormatContext(const char *inCharPath, Player *pPlayer);

void freeObjectMethod(Player* player);

void initCodecContext(const Player *pPlayer, const AVFormatContext *pContext);

void prepareVideoDecode(Player *pPlayer, JNIEnv *pInterface, jobject surface);

void initInputFormatContext(const char *inCharPath, Player *pPlayer) {
    int ref;
    av_register_all();
    AVFormatContext * pContext = avformat_alloc_context();

    ref=avformat_open_input(&pContext,inCharPath,NULL,NULL);
    if (ref<0)
    {
        ERR("打开文件失败");
        return;
    }

    pPlayer->mformatContext=pContext;

    ref= avformat_find_stream_info(pContext,NULL);
    if (ref<0)
    {
        ERR("找不到流信息");
        return;
    }


    for (int i = 0; i < pContext->nb_streams; ++i) {
        if (pContext->streams[i]->codec->codec_type==AVMEDIA_TYPE_AUDIO)
        {
            pPlayer->mAudioIndex=i;
        } else if (pContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            pPlayer->mVideoIndex=i;
        }
    }

}

void initCodecContext(const Player *pPlayer,  int index) {

    int ref;

    AVFormatContext * pContext = pPlayer->mformatContext;

    AVCodecContext * mCodecontext = pContext->streams[index]->codec;


    const enum AVCodecID id = mCodecontext->codec_id;

    AVCodec * pCodec = avcodec_find_decoder(id);

    ref= avcodec_open2(mCodecontext,pCodec,NULL);
    if (ref<0)
    {
        ERR("找不到解码器");
    }

    pPlayer->codecContexts[index]=mCodecontext;
}

void prepareVideoDecode(Player *pPlayer, JNIEnv *env, jobject surface) {


//    av_read_frame(pPlayer->mformatContext,)
}

void freeObjectMethod(Player* player) {
    avformat_free_context(&(player->mformatContext));
}

JNIEXPORT  void JNICALL Java_com_palyer_wz1_bhplayer_VideoPlayer_ccPlayStream
(JNIEnv *env, jobject jobject, jstring js,jobject surface){

    const char * inCharPath = (*env)->GetStringUTFChars(env, js, NULL);

    //分配Player结构体
    Player * player = (Player*)malloc(sizeof(Player*));
    initInputFormatContext(inCharPath, player);

    //视频索引
    //音频索引

    //获取音频视频解码器 打开
    const int audioIndex = player->mAudioIndex;
    const int videoIndex = player->mVideoIndex;
    //封装格式上下文
    initCodecContext(player,audioIndex);
    initCodecContext(player,videoIndex);

    //准备视频解码
    prepareVideoDecode(player, env, surface);

    //准备音频解码

    //jni 音频准备

    //生成队列

    //线程读流

    //消费者线程解码

    //释放方法
    freeObjectMethod(player);
}
