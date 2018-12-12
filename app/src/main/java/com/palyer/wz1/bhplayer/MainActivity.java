package com.palyer.wz1.bhplayer;

import android.Manifest;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.support.annotation.NonNull;
import android.support.v4.app.ActivityCompat;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.widget.TextView;

import java.io.File;

public class MainActivity extends AppCompatActivity implements SurfaceHolder.Callback {

    private static final int REQUEST_PERMISSION_CODE = 0x2545;
    private static String[] PERMISSIONS_STORAGE = {
            Manifest.permission.READ_EXTERNAL_STORAGE,
            Manifest.permission.WRITE_EXTERNAL_STORAGE,
            Manifest.permission.RECORD_AUDIO
    };

    private SurfaceView surface;
    private static final String TAG = "MainActivity";

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
//        mHolder.addCallback(this);
        // Example of a call to a native method
        TextView tv = (TextView) findViewById(R.id.sample_text);
        tv.setText(stringFromJNI());
        videoPlayer = new VideoPlayer();
        permissionsMethod();
//        audioDecoder();
        testpthread();
    }

    private void testpthread() {
        videoPlayer.pThreadTest();
    }

    //音频解码
    private void audioDecoder(){
        String inputPath = new File(Environment.getExternalStorageDirectory(), "说散就散.mp3").getAbsolutePath();
        String pcmPath = new File(Environment.getExternalStorageDirectory(), "小苹果.pcm").getAbsolutePath();
        videoPlayer.decodeAudio(inputPath,pcmPath);
    }

    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    public native String stringFromJNI();


    public void permissionsMethod(){

        if (Build.VERSION.SDK_INT > Build.VERSION_CODES.LOLLIPOP) {
            if (ActivityCompat.checkSelfPermission(this, Manifest.permission.RECORD_AUDIO) != PackageManager.PERMISSION_GRANTED) {
                ActivityCompat.requestPermissions(this, PERMISSIONS_STORAGE, REQUEST_PERMISSION_CODE);
            }
        }

    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (requestCode == REQUEST_PERMISSION_CODE) {
            for (int i = 0; i < permissions.length; i++) {
                Log.e("MainActivity", "申请的权限为：" + permissions[i] + ",申请结果：" + grantResults[i]);
            }
        }
    }

    @Override
    public void surfaceCreated(final SurfaceHolder holder) {
        new Thread(new Runnable() {
            @Override
            public void run() {
                Surface surface = holder.getSurface();
                File file = new File(Environment.getExternalStorageDirectory(), "小苹果.mp4");
                if (!file.exists())
                {
                    Log.e(TAG, "run:文件不存在 " );
                }
                String absolutePath = file.getAbsolutePath();

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
