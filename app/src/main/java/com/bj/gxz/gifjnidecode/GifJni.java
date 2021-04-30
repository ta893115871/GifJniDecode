package com.bj.gxz.gifjnidecode;

import android.graphics.Bitmap;

/**
 * Created by guxiuzhong on 2020/11/24.
 */
public class GifJni {

    static {
        System.loadLibrary("native-lib");
    }

    // Convenience for JNI access
    public long mNativePtr;


    public GifJni(String path) {
        this.mNativePtr = loadPath(path);
    }

    public int getWidth(){
        return getWidth(mNativePtr);
    }

    public int getHeight(){
        return getHeight(mNativePtr);
    }

    public int updateFrame(Bitmap bitmap){
        return updateFrame(mNativePtr,bitmap);
    }

    //通过路径加载gif图片(这里使用的是本地图片,源码中的gif加载是支持流的格式的)
    public native long loadPath(String path);

    //获取gif的宽
    public native int getWidth(long mNativePtr);

    //获取gif的高
    public native int getHeight(long mNativePtr);

    //每隔一段时间刷新一次,返回的int值表示下次刷新的时间间隔
    public native int updateFrame(long mNativePtr, Bitmap bitmap);
}
