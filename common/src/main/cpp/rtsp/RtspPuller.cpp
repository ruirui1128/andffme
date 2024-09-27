#include <jni.h>
#include <android/log.h>
#include <unistd.h>  // 用于 sleep 函数
#include "include/RtspNativeCallBack.h"
#include "include/RtspPlayer.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/time.h>
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
}

#define LOGI(...) __android_log_print(ANDROID_LOG_ERROR, "FFmpegRTSP", __VA_ARGS__)


AVFormatContext *fmt_ctx = nullptr; //  RTSP流上下文
AVCodecContext *video_codec_ctx = nullptr; // 视频编码器上下文
AVCodecContext *audio_codec_ctx = nullptr; // 视频编码器上下文
SwrContext *swr_ctx = nullptr; // 音频采样器上下文
SwrContext *sws_ctx = nullptr; // 视频采样器上下文
AVStream *video_steam = nullptr; // 视频流
AVStream *audio_steam = nullptr; // 音频流
int video_stream_index = -1; // 视频索引
int audio_stream_index = -1; // 音频索引

int64_t audio_clock = 0;  // 音频播放时的时间戳，单位为微秒

bool isPaused = false; // 控制拉流 是否暂停
bool isStopped = false; // 控制拉流是否停止


JNIEnv *g_env;  // 全局 jniEnv 指针
jobject g_obj;  // 全局对象指针
jmethodID videoFrameCallback;  // Java 层视频帧回调函数
jmethodID audioFrameCallback;  // Java 层音频帧回调函数

ANativeWindow *nativeWindow = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; // 静态初始化 互斥锁


void renderVideoFrame(uint8_t *src_data, int width, int height, int src_size) {
    pthread_mutex_lock(&mutex);

    if (!nativeWindow) {
        pthread_mutex_unlock(&mutex);
        nativeWindow = 0;
        return;
    }

    //设置窗口属性
    ANativeWindow_setBuffersGeometry(nativeWindow, width, height, WINDOW_FORMAT_RGBA_8888);

    ANativeWindow_Buffer window_buffer;

    if (ANativeWindow_lock(nativeWindow, &window_buffer, 0)) {
        ANativeWindow_release(nativeWindow);
        nativeWindow = 0;
        pthread_mutex_unlock(&mutex);
        return;
    }

    //填数据到 buffer,其实就是修改数据
    uint8_t *dst_data = static_cast<uint8_t *>(window_buffer.bits);
    int lineSize = window_buffer.stride * 4;//RGBA

    //下面就是逐行 copy 了。
    //一行 copy
    for (int i = 0; i < window_buffer.height; ++i) {
        memcpy(dst_data + i * lineSize, src_data + i * src_size, lineSize);
    }
    ANativeWindow_unlockAndPost(nativeWindow);
    pthread_mutex_unlock(&mutex);
}


int64_t get_current_audio_pts() {
    // 获取音频的当前播放时间 (microsecond)
    return audio_clock;
}

// 音视频同步：将当前视频帧的 pts 与音频的 pts 进行对比，确保同步播放
void synchronize_video(int64_t video_pts) {
    int64_t current_audio_pts = get_current_audio_pts();  // 获取当前音频的播放时间
    // 如果视频帧的时间比音频帧播放的时间戳早 则需要等待
    if (video_pts > current_audio_pts) {
        int64_t delay = video_pts - current_audio_pts;
        if (delay > 0) {
            usleep(delay);  // 延迟等待，以实现音视频同步
        }
    }
}


extern "C"
JNIEXPORT void JNICALL
Java_com_dwayne_com_rtsp_RtspPull_initRTSPStream(JNIEnv *env, jobject thiz, jstring url) {
    const char *rtsp_url = env->GetStringUTFChars(url, nullptr);  // 获取视频流
    g_env = env;
    g_obj = env->NewGlobalRef(thiz);  // 创建全局引用

    fmt_ctx = avformat_alloc_context();

    avformat_network_init();  // 初始化网络组件

    // 打卡steam流
    if (avformat_open_input(&fmt_ctx, rtsp_url, nullptr, nullptr)) {
        LOGI("Failed to open RTSP stream");
        return;
    }
    // 获取流信息
    if (avformat_find_stream_info(fmt_ctx, nullptr) < 0) {
        LOGI("Failed to find stream info");
        return;
    }
    //  遍历流信息 找到音视频流
    for (int i = 0; i < fmt_ctx->nb_streams; ++i) {
        AVCodecParameters *codec_par = fmt_ctx->streams[i]->codecpar;
        AVCodec *codec = avcodec_find_decoder(codec_par->codec_id);
        if (codec_par->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_index = i;
            video_codec_ctx = avcodec_alloc_context3(codec);  // 给视频流分配编解码器
            avcodec_parameters_to_context(video_codec_ctx, codec_par); //将编解码参数转换为编解码上下文
            avcodec_open2(video_codec_ctx, codec, nullptr);  // 打开视频解码器
        } else if (codec_par->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_stream_index = i;
            audio_codec_ctx = avcodec_alloc_context3(codec);// 为音频流分配编解码器
            avcodec_parameters_to_context(audio_codec_ctx, codec_par); // 将编解码参数转换为编解码上下文
            avcodec_open2(audio_codec_ctx, codec, nullptr); // 打开音频解码器
        }
    }

    // 获取java方法ID 用于回调音视频帧
    videoFrameCallback = env->GetMethodID(env->GetObjectClass(thiz), "onVideoFrame", "([BIII)V");
    audioFrameCallback = env->GetMethodID(env->GetObjectClass(thiz), "onAudioFrame", "([BI)V");

    env->ReleaseStringUTFChars(url, rtsp_url);  // 释放 URL 字符串

    LOGI("========初始化成功=======");
}
extern "C"
JNIEXPORT void JNICALL
Java_com_dwayne_com_rtsp_RtspPull_pullVideoData(JNIEnv *env, jobject thiz) {
    AVPacket *packet = av_packet_alloc();
    AVFrame *frame = av_frame_alloc();  // 视频帧
    while (av_read_frame(fmt_ctx, packet) >= 0 && !isStopped) {
        if (isPaused) continue;
        if (packet->stream_index == video_stream_index) {  // 如果为视频流
            avcodec_send_packet(video_codec_ctx, packet);  // 发送视频包到解码器
            while (avcodec_receive_frame(video_codec_ctx, frame)) {  // 解码器接收到视频帧

                if (frame->data[0] == NULL) {
                    LOGI("Decoded frame data is NULL");
                    continue;  // 跳过当前循环
                }

                // 获取视频帧的 pts（单位是时间基time_base的倍数）
                int64_t video_pts = av_rescale_q(frame->pts, video_codec_ctx->time_base,
                                                 AV_TIME_BASE_Q);
                synchronize_video(video_pts);  // 同步视频播放

                int width = video_codec_ctx->width;
                int height = video_codec_ctx->height;
                // 转换为适配 MediaCodec 的格式，例如 YUV420P
                jbyteArray video_data = env->NewByteArray(
                        frame->linesize[0] * video_codec_ctx->height);  // 创建 Java 层的 byte 数组
                env->SetByteArrayRegion(video_data, 0, frame->linesize[0] * video_codec_ctx->height,
                                        (jbyte *) frame->data[0]);  // 将视频帧数据传递给 Java 层
                env->CallVoidMethod(g_obj, videoFrameCallback, video_data, frame->linesize[0] *
                                                                           video_codec_ctx->height,
                                    width, height);  // 调用 Java 层的回调函数
                env->DeleteLocalRef(video_data);  // 释放局部引用
            }
        }
        av_packet_unref(packet);  // 释放包
    }
    av_frame_free(&frame);  // 释放视频帧

}

extern "C"
JNIEXPORT void JNICALL
Java_com_dwayne_com_rtsp_RtspPull_pullAudioData(JNIEnv *env, jobject thiz) {
    AVPacket *packet = av_packet_alloc();  // 音频包
    AVFrame *frame = av_frame_alloc();  // 音频帧

    while (av_read_frame(fmt_ctx, packet) >= 0 && !isStopped) {
        if (isPaused) continue;  // 如果暂停拉流，跳过处理

        if (packet->stream_index == audio_stream_index) {  // 判断是否为音频流
            avcodec_send_packet(audio_codec_ctx, packet);  // 发送音频包到解码器
            while (avcodec_receive_frame(audio_codec_ctx, frame) >= 0) {  // 接收解码后的音频帧

                int64_t audio_pts = av_rescale_q(frame->pts, audio_codec_ctx->time_base,
                                                 AV_TIME_BASE_Q);
                // 更新音频播放时钟
                audio_clock = audio_pts;
                // 转换为适配 AudioTrack 的 PCM 格式
                jbyteArray audio_data = env->NewByteArray(
                        frame->linesize[0]);  // 创建 Java 层的 byte 数组
                env->SetByteArrayRegion(audio_data, 0, frame->linesize[0],
                                        (jbyte *) frame->data[0]);  // 将音频帧数据传递给 Java 层
                env->CallVoidMethod(g_obj, audioFrameCallback, audio_data,
                                    frame->linesize[0]);  // 调用 Java 层的回调函数
                env->DeleteLocalRef(audio_data);  // 释放局部引用
            }
        }
        av_packet_unref(packet);  // 释放包
    }

    av_frame_free(&frame);  // 释放音频帧
}

extern "C"
JNIEXPORT void JNICALL
Java_com_dwayne_com_rtsp_RtspPull_pauseRTSPStream(JNIEnv *env, jobject thiz) {
    isPaused = true;  // 设置暂停标志
}
extern "C"
JNIEXPORT void JNICALL
Java_com_dwayne_com_rtsp_RtspPull_resumeRTSPStream(JNIEnv *env, jobject thiz) {
    isPaused = false;  // 取消暂停标志
}

extern "C"
JNIEXPORT void JNICALL
Java_com_dwayne_com_rtsp_RtspPull_stopRTSPStream(JNIEnv *env, jobject thiz) {
    isStopped = true;  // 设置停止标志

    // 写入流尾部数据并关闭 RTSP 流
    if (fmt_ctx) {
        avformat_close_input(&fmt_ctx);
    }

    // 释放编解码器上下文
    if (video_codec_ctx) {
        avcodec_free_context(&video_codec_ctx);
    }
    if (audio_codec_ctx) {
        avcodec_free_context(&audio_codec_ctx);
    }

    // 清理网络组件
    avformat_network_deinit();

    // 释放全局 Java 对象
    if (g_obj) {
        env->DeleteGlobalRef(g_obj);
    }
}


JavaVM *javaVM = 0;
RtspPlayer *player = 0;


int JNI_OnLoad(JavaVM *javaVM1, void *pVoid) {
    ::javaVM = javaVM1;
    return JNI_VERSION_1_6;
}


extern "C"
JNIEXPORT void JNICALL
Java_com_dwayne_com_rtsp_RtspPull_initPrepeare(JNIEnv *env, jobject thiz, jstring mDataSource_) {
    // 准备工作的话，首先要来解封装
    RtspNativeCallBack *jniCallback = new RtspNativeCallBack(javaVM, env, thiz);
    //转成 C 字符串
    const char *data_source = env->GetStringUTFChars(mDataSource_, NULL);
    player = new RtspPlayer(data_source, jniCallback);
    player->prepare();
    player->setVideoRenderCallback(renderVideoFrame);
    env->ReleaseStringUTFChars(mDataSource_, data_source);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_dwayne_com_rtsp_RtspPull_startRtsp(JNIEnv *env, jobject thiz) {
    if (player) {
        player->start();
    }
}

/**
 * 设置播放 surface
 */
extern "C"
JNIEXPORT void JNICALL
Java_com_dwayne_com_rtsp_RtspPull_setSurfaceNative(JNIEnv *env, jclass type,
                                                   jobject surface) {
    LOGD("Java_com_dwayne_com_player_PlayerManager_setSurfaceNative");
    pthread_mutex_lock(&mutex);
    if (nativeWindow) {
        ANativeWindow_release(nativeWindow);
        nativeWindow = 0;
    }
    //创建新的窗口用于视频显示窗口
    nativeWindow = ANativeWindow_fromSurface(env, surface);
    pthread_mutex_unlock(&mutex);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_dwayne_com_rtsp_RtspPull_releaseRtsp(JNIEnv *env, jobject thiz) {
    if (player) {
        player->release();
    }
    if (nativeWindow) {
        ANativeWindow_release(nativeWindow);
        nativeWindow = nullptr;
    }


}
