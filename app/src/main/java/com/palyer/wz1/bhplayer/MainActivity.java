package com.palyer.wz1.bhplayer;

import android.os.Bundle;
import android.os.Environment;
import android.support.v7.app.AppCompatActivity;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.widget.TextView;

import java.io.File;

public class MainActivity extends AppCompatActivity implements SurfaceHolder.Callback {

    private SurfaceView surface;

    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("native-lib");
    }

    private VideoPlayer videoPlayer;
    private SurfaceHolder mHolder;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        surface=(SurfaceView)findViewById(R.id.surface);
        mHolder = surface.getHolder();
        mHolder.addCallback(this);
        // Example of a call to a native method
        TextView tv = (TextView) findViewById(R.id.sample_text);
        tv.setText(stringFromJNI());
        videoPlayer = new VideoPlayer();
    }

    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    public native String stringFromJNI();

    @Override
    public void surfaceCreated(final SurfaceHolder holder) {
        new Thread(new Runnable() {
            @Override
            public void run() {
                Surface surface = holder.getSurface();
                String absolutePath = new File(Environment.getExternalStorageDirectory(), "小苹果.mp3").getAbsolutePath();
                videoPlayer.render(absolutePath,surface);
            }
        }).start();
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {

    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        holder.getSurface().release();
    }
}
