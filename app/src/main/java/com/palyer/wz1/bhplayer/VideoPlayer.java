package com.palyer.wz1.bhplayer;

import android.view.Surface;

/**
 * Created by Administrator on 2018-12-10.
 * <p>
 * by author wz
 * <p>
 * com.palyer.wz1.bhplayer
 */
public class VideoPlayer {
    //视频播放
    public native void render(String input,Surface surface);
    static{
        System.loadLibrary("avutil-54");
        System.loadLibrary("swresample-1");
        System.loadLibrary("avcodec-56");
        System.loadLibrary("avformat-56");
        System.loadLibrary("swscale-3");
        System.loadLibrary("postproc-53");
        System.loadLibrary("avfilter-5");
        System.loadLibrary("avdevice-56");
        System.loadLibrary("native-lib");
    }
}
