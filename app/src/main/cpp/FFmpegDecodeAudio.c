//
// Created by Administrator on 2018-12-11.
//

#include <jni.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <unistd.h>
#include "android/log.h"
#include "libavutil/channel_layout.h"
//解码
#include "include/libavcodec/avcodec.h"

#define LOG_TAG "FFmpegDecodeAudio"

#define LOG_ERR(FORMAT,...)   __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,FORMAT,__VA_ARGS__)
#define ERR(String)  LOG_ERR("%s",String)

#define MAX_AUDIO_FRME_SIZE  2 * 44100


JNIEXPORT void JNICALL Java_com_palyer_wz1_bhplayer_VideoPlayer_decodeAudio
(JNIEnv *env, jobject jobject1, jstring inString, jstring outString){
    ERR("测试打印LOG");
    const char *inPathChar = (*env)->GetStringUTFChars(env, inString, NULL);
    const char *outPathChar = (*env)->GetStringUTFChars(env, outString, NULL);


    ERR("init");
    av_register_all();

    //formatContext
    AVFormatContext *pFormatContext = avformat_alloc_context();
    LOG_ERR("打开文件%s",inPathChar);
    //打开音频文件
    if (avformat_open_input(&pFormatContext,inPathChar,NULL,NULL)!=0)
    {
        ERR("打开音频文件失败");
        return;
    }

    if (avformat_find_stream_info(pFormatContext,NULL)<0)
    {
        ERR("获取音频文件信息失败");
        return;
    }

    int streamIndex=-1,i=0;
    for (; i < pFormatContext->nb_streams; ++i) {
        if (pFormatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO )
        {
            streamIndex =i;
            break;
        }
    }

    if (streamIndex == -1)
    {
        ERR("找不到音频流");
        return;
    }
    AVCodecContext *avCodecContext = pFormatContext->streams[streamIndex]->codec;

    enum AVCodecID id = avCodecContext->codec_id;

    AVCodec *pCodec = avcodec_find_decoder(id);

    if (avcodec_open2(avCodecContext,pCodec,NULL)<0)
    {
        ERR("找不到codec，找不到解码器");
        return;
    }

     AVPacket* packet  = (  AVPacket *)av_malloc(sizeof(AVPacket));

    AVFrame * pFrame = av_frame_alloc();

    struct SwrContext * swrContext = swr_alloc();


    //输入采样率格式
    enum AVSampleFormat inSampleFormat = avCodecContext->sample_fmt;

    //输出采样率格式
    enum AVSampleFormat outSampleFormat = AV_SAMPLE_FMT_S16;

    //输入采样率
    const int inRate = avCodecContext->sample_rate;

    //输出采样率
    const int outRate = 44100;

    //获取输入声道布局
    const uint64_t inuint64 = avCodecContext->channel_layout;

    //获取输出声道布局
    const uint64_t outuint64 = AV_CH_LAYOUT_2_1;

    swr_alloc_set_opts(swrContext,outuint64,outSampleFormat,outRate,inuint64,inSampleFormat,inRate,0,NULL);

    swr_init(swrContext);

    const int channels = av_get_channel_layout_nb_channels(outuint64);
    LOG_ERR("声道为%d",channels);

    jclass  aClass = (*env)->FindClass(env, "com/palyer/wz1/bhplayer/AudioUtil");

    jmethodID const methodID = (*env)->GetMethodID(env, aClass, "<init>", "()V");

    jobject const audioObj = (*env)->NewObject(env, aClass, methodID);

    jmethodID const audioTrackObj1 = (*env)->GetMethodID(env, aClass, "createAudioTrack", "(II)Landroid/media/AudioTrack;");

    jobject const callAudioTrackObject = (*env)->CallObjectMethod(env, audioObj, audioTrackObj1, outRate, channels);

    jclass const audioTrackClass = (*env)->GetObjectClass(env, callAudioTrackObject);

    jmethodID const objPalyID = (*env)->GetMethodID(env, audioTrackClass, "play", "()V");

    (*env)->CallVoidMethod(env,callAudioTrackObject,objPalyID);

    jmethodID const audiotrack_write_pID = (*env)->GetMethodID(env, audioTrackClass, "write", "([BII)I");



    uint8_t *out_buffer = (uint8_t *)av_malloc(MAX_AUDIO_FRME_SIZE);

    FILE *const outFILE = fopen(outPathChar, "wb");

    int goto_frame =0,framecount=0,ret;
    while (av_read_frame(pFormatContext,packet)>=0)
    {
        if (packet->stream_index==streamIndex){

           ret= avcodec_decode_audio4(avCodecContext,pFrame,&goto_frame,packet);
            if (ret<0)
            {
                ERR("解码完成");
                break;
            }

            if (goto_frame>0)
            {
                LOG_ERR("解码 %d",framecount++);
                //转成输出声道的格式
                swr_convert(swrContext, &out_buffer,MAX_AUDIO_FRME_SIZE,
                            (const uint8_t **) pFrame->data, pFrame->nb_samples);
                const int outBufferSize = av_samples_get_buffer_size(NULL, channels, pFrame->nb_samples, outSampleFormat, 1);

                jbyteArray  jbufferArray = (*env)->NewByteArray(env, outBufferSize);

                jbyte * jbufferbyte = (*env)->GetByteArrayElements(env, jbufferArray, NULL);

                memcpy(jbufferbyte, out_buffer,outBufferSize);

                (*env)->ReleaseByteArrayElements(env,jbufferArray,jbufferbyte,0);

                (*env)->CallIntMethod(env,callAudioTrackObject,audiotrack_write_pID,jbufferArray,0,outBufferSize);

                (*env)->DeleteLocalRef(env,jbufferArray);
//                fwrite(out_buffer, 1, (size_t) outBufferSize, outFILE);
                usleep(1000 * 16); //16ms
            }
        }
        av_free_packet(packet);
    }
    fclose(outFILE);

    av_frame_free(&pFrame);
    free(out_buffer);
    swr_free(&swrContext);
    avcodec_close(avCodecContext);
    avformat_close_input(&pFormatContext);

    (*env)->ReleaseStringUTFChars(env,inString,inPathChar);
    (*env)->ReleaseStringUTFChars(env,outString,outPathChar);
}