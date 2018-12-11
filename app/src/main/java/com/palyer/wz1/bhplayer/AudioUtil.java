package com.palyer.wz1.bhplayer;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.util.Log;

/**
 * Created by Administrator on 2018-12-11.
 * <p>
 *
 *public class com.palyer.wz1.bhplayer.AudioUtil {
 private static final java.lang.String TAG;
 descriptor: Ljava/lang/String;
 public com.palyer.wz1.bhplayer.AudioUtil();
 descriptor: ()V

 public int write(byte[], int, int);
 descriptor: ([BII)I

 public android.media.AudioTrack createAudioTrack(int, int);
 descriptor: (II)Landroid/media/AudioTrack;
 }

 * <p>
 * com.palyer.wz1.bhplayer
 */
public class AudioUtil {
    
    private static final String TAG = "AudioUtil";

//    public int write(byte[] audioData, int offsetInBytes, int sizeInBytes) {
//        return 0;
//    }

    /**
     * 创建一个AudioTrack对象，用于播放
     */
    public AudioTrack createAudioTrack(int sampleRateInHz,int nb_channels)
    {
        Log.e(TAG, "sampleRateInHz: "+sampleRateInHz);
        Log.e(TAG, "nb_channels: "+nb_channels);

        int audioFormat = AudioFormat.ENCODING_PCM_16BIT;

        //声道个数 影响声道的布局
        int channelConfig;
        if (nb_channels==1)
        {
            channelConfig=AudioFormat.CHANNEL_OUT_MONO;
        }else if (nb_channels ==2)
        {
            channelConfig=AudioFormat.CHANNEL_OUT_STEREO;
        }else {
            channelConfig=AudioFormat.CHANNEL_OUT_STEREO;
        }

        int minBufferSizeByte = AudioTrack.getMinBufferSize(sampleRateInHz, channelConfig, audioFormat);

        AudioTrack audioTrack = new AudioTrack(AudioManager.STREAM_MUSIC, sampleRateInHz, channelConfig, audioFormat, minBufferSizeByte, AudioTrack.MODE_STREAM);
        return audioTrack;
    }

}
