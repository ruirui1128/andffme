#include <jni.h>

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include "rtmp/rtmp.h"

#include <vector>
#include <android/log.h>
#include <android/native_window_jni.h>



extern "C" {
#include "libavutil/time.h"
}


extern "C" {
#include "libavformat/avformat.h"
}

extern "C" {
#include <libavcodec/avcodec.h>
}

extern "C" {
#include "libavutil/imgutils.h"
}
extern "C" {
#include <libswscale/swscale.h>
}
#include <thread>  // 引入多线程库
#include <atomic>  // 使用原子类型来管理解码线程状态

std::thread decodeThread;
std::atomic<bool> isPlaying(true);  // 使用原子类型管理播放状态


#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,"FrankLive",__VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,"FrankLive",__VA_ARGS__)
#define BUFF_SIZE 1024 * 1024


extern "C"
JNIEXPORT void JNICALL
Java_com_mind_andffme_pull_RtmpPullStreamPlayer_pullRTMPStream2(JNIEnv *env, jobject thiz,
                                                                jstring url, jobject surface) {
    const char *path = env->GetStringUTFChars(url, nullptr);
    char *urlSource = new char[strlen(path) + 1];
    strcpy(urlSource, path);
    env->ReleaseStringUTFChars(url, path);

    // 初始化网络模块
    avformat_network_init();

    // 打开输入流
    AVFormatContext *formatContext = avformat_alloc_context();
    if (avformat_open_input(&formatContext, urlSource, NULL, NULL) != 0) {
        LOGE("无法打开输入流");
        delete[] urlSource;
        return;
    }

    if (avformat_find_stream_info(formatContext, NULL) < 0) {
        LOGE("无法获取流信息");
        avformat_close_input(&formatContext);
        delete[] urlSource;
        return;
    }

    // 查找视频流索引
    int videoStreamIndex = -1;
    for (int i = 0; i < formatContext->nb_streams; i++) {
        if (formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStreamIndex = i;
            break;
        }
    }

    if (videoStreamIndex == -1) {
        LOGE("未找到视频流");
        avformat_close_input(&formatContext);
        delete[] urlSource;
        return;
    }

    // 初始化编解码器
    AVCodecParameters *codecParams = formatContext->streams[videoStreamIndex]->codecpar;
    const AVCodec *codec = avcodec_find_decoder(codecParams->codec_id);
    AVCodecContext *codecContext = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(codecContext, codecParams);
    if (avcodec_open2(codecContext, codec, NULL) < 0) {
        LOGE("无法打开编解码器");
        avcodec_free_context(&codecContext);
        avformat_close_input(&formatContext);
        delete[] urlSource;
        return;
    }

    // 设置ANativeWindow的宽高
    ANativeWindow *nativeWindow = ANativeWindow_fromSurface(env, surface);
    if (!nativeWindow) {
        LOGE("无法获取 ANativeWindow");
        avcodec_free_context(&codecContext);
        avformat_close_input(&formatContext);
        delete[] urlSource;
        return;
    }
    ANativeWindow_setBuffersGeometry(nativeWindow, codecContext->width, codecContext->height, WINDOW_FORMAT_RGBA_8888);
    ANativeWindow_Buffer windowBuffer;

    // 初始化SwsContext
    struct SwsContext *swsContext = sws_getContext(
            codecContext->width, codecContext->height, codecContext->pix_fmt,
            codecContext->width, codecContext->height, AV_PIX_FMT_RGBA,
            SWS_BILINEAR, NULL, NULL, NULL);

    // 分配帧
    AVFrame *frame = av_frame_alloc();
    AVFrame *rgbFrame = av_frame_alloc();
    uint8_t *buffer = (uint8_t *) av_malloc(av_image_get_buffer_size(AV_PIX_FMT_RGBA, codecContext->width, codecContext->height, 1));
    av_image_fill_arrays(rgbFrame->data, rgbFrame->linesize, buffer, AV_PIX_FMT_RGBA, codecContext->width, codecContext->height, 1);

    AVPacket *packet = av_packet_alloc();

    isPlaying = true; // 启动播放

    decodeThread = std::thread([&]() {
        while (isPlaying && av_read_frame(formatContext, packet) >= 0) {
            if (packet->stream_index == videoStreamIndex) {
                if (avcodec_send_packet(codecContext, packet) == 0) {
                    if (avcodec_receive_frame(codecContext, frame) == 0) {
                        // 转换YUV到RGBA
                        sws_scale(swsContext, (const uint8_t *const *) frame->data, frame->linesize, 0, codecContext->height,
                                  rgbFrame->data, rgbFrame->linesize);

                        // 锁定ANativeWindow
                        if (ANativeWindow_lock(nativeWindow, &windowBuffer, NULL) == 0) {
                            uint8_t *dst = (uint8_t *) windowBuffer.bits;
                            int dstStride = windowBuffer.stride * 4;
                            uint8_t *src = rgbFrame->data[0];
                            int srcStride = rgbFrame->linesize[0];

                            // 拷贝图像数据
                            for (int h = 0; h < codecContext->height; h++) {
                                memcpy(dst + h * dstStride, src + h * srcStride, srcStride);
                            }

                            ANativeWindow_unlockAndPost(nativeWindow);
                        } else {
                            LOGE("无法锁定 ANativeWindow");
                        }
                    }
                }
            }
            av_packet_unref(packet);  // 释放packet
        }

        // 清理资源
        av_frame_free(&frame);
        av_frame_free(&rgbFrame);
        av_packet_free(&packet);
        avcodec_free_context(&codecContext);
        avformat_close_input(&formatContext);
        avformat_network_deinit();
        av_free(buffer);
        ANativeWindow_release(nativeWindow);
        delete[] urlSource;
    });

    decodeThread.join();  // 分离线程，防止主线程被阻塞
}

