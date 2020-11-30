#include <jni.h>
#include <string>
#include <android/bitmap.h>
#include <android/log.h>
#include "gif_lib.h"

#define  LOG_TAG "GIF"
#define  LOGD(...) __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__)

#define  argb(a, r, g, b) ( ((a) & 0xff) << 24 ) | ( ((b) & 0xff) << 16 ) | ( ((g) & 0xff) << 8 ) | ((r) & 0xff)

#define  dispose(ext) (((ext)->Bytes[0] & 0x1c) >> 2)
#define  trans_index(ext) ((ext)->Bytes[3])
#define  transparency(ext) ((ext)->Bytes[0] & 1)

typedef struct GifBean {
    // 当前帧
    int current_frame;
    // 总帧数
    int total_frame;
    // 延迟时间数组,长度不确定,根据gif帧数计算
    int *delays;
} GifBean;

void drawFrame(GifFileType *pType, GifBean *pBean, AndroidBitmapInfo info, void *pVoid);

extern "C"
JNIEXPORT jint JNICALL
Java_com_bj_gxz_gifjnidecode_GifJni_getWidth(JNIEnv *env, jobject thiz, jlong native_address_ptr) {
    GifFileType *gifFileType = reinterpret_cast<GifFileType *>(native_address_ptr);

    return gifFileType->SWidth;
}


extern "C"
JNIEXPORT jint JNICALL
Java_com_bj_gxz_gifjnidecode_GifJni_getHeight(JNIEnv *env, jobject thiz, jlong native_address_ptr) {

    GifFileType *gifFileType = reinterpret_cast<GifFileType *>(native_address_ptr);

    return gifFileType->SHeight;
}


extern "C"
JNIEXPORT jlong JNICALL
Java_com_bj_gxz_gifjnidecode_GifJni_loadPath(JNIEnv *env, jobject thiz, jstring path) {

    const char *c_path = (char *) env->GetStringUTFChars(path, 0);

    int error;
    // 打开gif文件
    GifFileType *gifFileType = DGifOpenFileName(c_path, &error);
    DGifSlurp(gifFileType);

    // 自定义一个bean类，来存储当前播放帧，总帧数，播放延迟的数组，并把bean对象与gifFileType绑定
    GifBean *gifBean = (GifBean *) malloc(sizeof(GifBean));
    memset(gifBean, 0, sizeof(GifBean));
    //初始化当前帧和总帧数
    gifBean->current_frame = 0;
    gifBean->total_frame = gifFileType->ImageCount;
    gifBean->delays = (int *) (malloc(sizeof(int) * gifFileType->ImageCount));
    memset(gifBean->delays, 0, sizeof(int) * gifFileType->ImageCount);

    // 把自己定义的数据结构保存到UserData，便于其它方法的获取使用
    gifFileType->UserData = gifBean;

    ExtensionBlock *extensionBlock;
    //遍历每一帧
    for (int i = 0; i < gifFileType->ImageCount; i++) {
        //遍历每一帧中的扩展块(度娘Gif编码)
        SavedImage savedImage = gifFileType->SavedImages[i];
        for (int j = 0; j < savedImage.ExtensionBlockCount; j++) {
            //取图形控制扩展块,其中包含延迟时间
            if (savedImage.ExtensionBlocks[j].Function == GRAPHICS_EXT_FUNC_CODE) {
                extensionBlock = &savedImage.ExtensionBlocks[j];
            }
        }
        //获取延迟时间,extensionBlock的第二,三个元素一起存放延迟时间低8位和高8位向左偏移8位,进行或运算
        //乘10因为编码的时间单位是1/100秒 乘10换算为毫秒
        if (extensionBlock) {
            int frame_delay = 10 * (extensionBlock->Bytes[1] | extensionBlock->Bytes[2] << 8);
            gifBean->delays[i] = frame_delay;
            // LOGD("delays[%d]=%d", i, frame_delay);
        }
    }
    env->ReleaseStringUTFChars(path, c_path);
    return reinterpret_cast<jlong>(gifFileType);
}


// 直接修改bitmap的像素
void drawFrame(GifFileType *gifFileType, GifBean *gifBean, AndroidBitmapInfo info, void *pixels) {

    // 先拿到当前帧
    SavedImage savedImage = gifFileType->SavedImages[gifBean->current_frame];


    GifImageDesc imageDesc = savedImage.ImageDesc;

    // 获取color map
    //字典,存放的是gif压缩rgb数据
    ColorMapObject *colorMap = imageDesc.ColorMap;
    //部分图片某些帧的ColorMapObject取到为null
    if (colorMap == NULL) {
        colorMap = gifFileType->SColorMap;
    }

    GifByteType gifByteType;
    int pointPixels;
    // bitmap的像素的指针
    int *px = (int *) pixels;
    // 在jni c层中真实存储是 A B G R
    int bitmapLineStart;    // bitmap每一行的首地址
    for (int y = imageDesc.Top; y < imageDesc.Top + imageDesc.Height; y++) {
        // 更新行的首地址
        bitmapLineStart = y * info.width;
        for (int x = imageDesc.Left; x < imageDesc.Left + imageDesc.Width; x++) {
            // 拿到一个坐标的位置索引 --> 数据
            pointPixels = (y - imageDesc.Top) * imageDesc.Width + (x - imageDesc.Left);
            // 解压 gif中为了节省内存rgb采用lzw压缩,所以取rgb信息需要解压
            // 通过index拿到的是一个压缩数据
            gifByteType = savedImage.RasterBits[pointPixels];
            // 拿到真正的rgb
            GifColorType gifColorType = colorMap->Colors[gifByteType];
            // 转成一个int值，并赋值给对应的像素点
            px[bitmapLineStart + x] =
                    argb(255, gifColorType.Red, gifColorType.Green, gifColorType.Blue);

        }

    }
}

int drawFrameNormal(GifFileType *gif, AndroidBitmapInfo info, void *pixels, bool force_dispose_1) {
    GifColorType *bg;

    GifColorType *color;

    SavedImage *frame;

    ExtensionBlock *ext = 0;

    GifImageDesc *frameInfo;

    ColorMapObject *colorMap;

    int *line;

    int width, height, x, y, j, loc, n, inc, p;

    void *px;

    GifBean *gifBean = static_cast<GifBean *>(gif->UserData);

    width = gif->SWidth;

    height = gif->SHeight;
    frame = &(gif->SavedImages[gifBean->current_frame]);

    frameInfo = &(frame->ImageDesc);

    if (frameInfo->ColorMap) {
        colorMap = frameInfo->ColorMap;
    } else {
        colorMap = gif->SColorMap;
    }
    bg = &colorMap->Colors[gif->SBackGroundColor];
    for (j = 0; j < frame->ExtensionBlockCount; j++) {
        if (frame->ExtensionBlocks[j].Function == GRAPHICS_EXT_FUNC_CODE) {
            ext = &(frame->ExtensionBlocks[j]);
            break;
        }
    }
    // For dispose = 1, we assume its been drawn
    px = pixels;
    if (ext && dispose(ext) == 1 && force_dispose_1 && gifBean->current_frame > 0) {
        gifBean->current_frame = gifBean->current_frame - 1;
        drawFrameNormal(gif, info, pixels, true);
    } else if (ext && dispose(ext) == 2 && bg) {
        for (y = 0; y < height; y++) {
            line = (int *) px;
            for (x = 0; x < width; x++) {
                line[x] = argb(255, bg->Red, bg->Green, bg->Blue);
            }
            px = (int *) ((char *) px + info.stride);
        }

    } else if (ext && dispose(ext) == 3 && gifBean->current_frame > 1) {
        gifBean->current_frame = gifBean->current_frame - 2;
        drawFrameNormal(gif, info, pixels, true);
    }
    px = pixels;
    if (frameInfo->Interlace) {
        n = 0;
        inc = 8;
        p = 0;
        px = (int *) ((char *) px + info.stride * frameInfo->Top);
        for (y = frameInfo->Top; y < frameInfo->Top + frameInfo->Height; y++) {
            for (x = frameInfo->Left; x < frameInfo->Left + frameInfo->Width; x++) {
                loc = (y - frameInfo->Top) * frameInfo->Width + (x - frameInfo->Left);
                if (ext && frame->RasterBits[loc] == trans_index(ext) && transparency(ext)) {
                    continue;
                }
                color = (ext && frame->RasterBits[loc] == trans_index(ext)) ? bg
                                                                            : &colorMap->Colors[frame->RasterBits[loc]];
                if (color) {
                    line[x] = argb(255, color->Red, color->Green, color->Blue);
                }
            }
            px = (int *) ((char *) px + info.stride * inc);
            n += inc;
            if (n >= frameInfo->Height) {
                n = 0;
                switch (p) {
                    case 0:
                        px = (int *) ((char *) pixels + info.stride * (4 + frameInfo->Top));
                        inc = 8;
                        p++;
                        break;
                    case 1:
                        px = (int *) ((char *) pixels + info.stride * (2 + frameInfo->Top));
                        inc = 4;
                        p++;
                        break;
                    case 2:
                        px = (int *) ((char *) pixels + info.stride * (1 + frameInfo->Top));
                        inc = 2;
                        p++;
                        break;
                    default:
                        break;
                }
            }
        }
    } else {
        px = (int *) ((char *) px + info.stride * frameInfo->Top);
        for (y = frameInfo->Top; y < frameInfo->Top + frameInfo->Height; y++) {
            line = (int *) px;
            for (x = frameInfo->Left; x < frameInfo->Left + frameInfo->Width; x++) {
                loc = (y - frameInfo->Top) * frameInfo->Width + (x - frameInfo->Left);
                if (ext && frame->RasterBits[loc] == trans_index(ext) && transparency(ext)) {
                    continue;
                }
                color = (ext && frame->RasterBits[loc] == trans_index(ext)) ? bg
                                                                            : &colorMap->Colors[frame->RasterBits[loc]];
                if (color) {
                    line[x] = argb(255, color->Red, color->Green, color->Blue);
                }
            }
            px = (int *) ((char *) px + info.stride);
        }
    }
    GraphicsControlBlock gcb;//获取控制信息
    DGifSavedExtensionToGCB(gif, gifBean->current_frame, &gcb);
    int delay = gcb.DelayTime * 10;
    return delay;
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_bj_gxz_gifjnidecode_GifJni_updateFrame(JNIEnv *env, jobject thiz, jlong native_address_ptr,
                                                jobject bitmap) {

    GifFileType *gifFileType = reinterpret_cast<GifFileType *>(native_address_ptr);
    GifBean *gifBean = (GifBean *) gifFileType->UserData;

    AndroidBitmapInfo info;
    AndroidBitmap_getInfo(env, bitmap, &info);

    void *addrPtr;
    AndroidBitmap_lockPixels(env, bitmap, &addrPtr);

//    drawFrame(gifFileType, gifBean, info, addrPtr);
    int delay = drawFrameNormal(gifFileType, info, addrPtr, false);

    // 循环播放
    gifBean->current_frame += 1;
    if (gifBean->current_frame >= gifBean->total_frame - 1) {
        gifBean->current_frame = 0;
    }

    AndroidBitmap_unlockPixels(env, bitmap);

    // 把下一帧的延迟时间返回上去
//    return gifBean->delays[gifBean->current_frame];
    return delay;
}