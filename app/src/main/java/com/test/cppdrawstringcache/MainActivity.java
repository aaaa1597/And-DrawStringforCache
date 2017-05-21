package com.test.cppdrawstringcache;

import android.app.Activity;
import android.graphics.PixelFormat;
import android.os.Bundle;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

public class MainActivity extends Activity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        Log.d("aaaaa", "aaaaa " + getPackageName());

        SurfaceView surface = ((SurfaceView)findViewById(R.id.surface));

        /* 透過設定 */
        surface.getHolder().setFormat(PixelFormat.TRANSLUCENT);
        surface.setZOrderOnTop(true);

        /* コールバック設定 */
        surface.getHolder().addCallback(new SurfaceHolder.Callback() {
            @Override
            public void surfaceCreated(SurfaceHolder holder) {
                /* C++ */
                NativeFunc.create(0);
                NativeFunc.surfaceCreated(0, holder.getSurface());
            }

            @Override
            public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
                NativeFunc.surfaceChanged(0, width, height);
            }

            @Override
            public void surfaceDestroyed(SurfaceHolder holder) {
                NativeFunc.surfaceDestroyed(0);
            }
        });

    }
}
