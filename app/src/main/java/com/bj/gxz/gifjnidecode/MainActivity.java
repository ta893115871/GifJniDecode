package com.bj.gxz.gifjnidecode;

import android.graphics.Bitmap;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.util.Log;
import android.view.View;
import android.widget.ImageView;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;

import java.io.File;

public class MainActivity extends AppCompatActivity {
    private GifJni gifJni;
    private ImageView imageView;
    private Bitmap bitmap;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        imageView = findViewById(R.id.id_iv);

//        File file = new File(Environment.getExternalStorageDirectory(), "demo.gif");
        // java
//        Glide.with( this )
//                .load( file.getAbsolutePath() )
//                .into( imageView );

        // c实现，建议
        // https://github.com/koral--/android-gif-drawable
//        try {
//            GifDrawable gifFromPath = new GifDrawable(file.getAbsolutePath());
//            imageView.setImageDrawable(gifFromPath);
//        } catch (IOException e) {
//            e.printStackTrace();
//        }
    }

    private final Handler handler = new Handler(Looper.getMainLooper()) {

        @Override
        public void handleMessage(@NonNull Message msg) {
            super.handleMessage(msg);
            updateFrame();
        }
    };

    // Android源码中的，有些gif解码有些问题
    public void ndkGif(View view) {
        File file = new File(Environment.getExternalStorageDirectory(), "demo.gif");
        gifJni = new GifJni(file.getAbsolutePath());
        int width = gifJni.getWidth();
        int height = gifJni.getHeight();
        Log.d("GIF", "width:" + width);
        Log.d("GIF", "height:" + height);
        bitmap = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
        Log.d("GIF", "bitmap:" + bitmap.getConfig().name());
        updateFrame();
    }

    private void updateFrame() {
        long cost = System.currentTimeMillis();
        //下一帧的刷新时间
        int delayShowTime = gifJni.updateFrame(bitmap);
        cost = System.currentTimeMillis() - cost;

        int realDelay = (int) (delayShowTime - cost);
        // 真正的延时，需要减去更新一帧所消耗的时间
        realDelay = Math.max(realDelay, 0);

        Log.d("GIF", "realDelay:" + realDelay + ",cost:" + cost);
        imageView.setImageBitmap(bitmap);
        handler.sendEmptyMessageDelayed(0, realDelay);
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        handler.removeCallbacksAndMessages(null);
    }
}