//
// Created by S on 2024/9/25.
//

#ifndef ANDFFME_RTSPPLAYER_H
#define ANDFFME_RTSPPLAYER_H

#include "RtspNativeCallBack.h"
#include "safe_queue.h"
#include "Constants.h"
#include <android/native_window_jni.h> // 是为了 渲染到屏幕支持的

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/time.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

typedef void (*VideoRenderCallback)(uint8_t *, int, int, int);

class RtspPlayer {
public:
    RtspPlayer();

    RtspPlayer(const char *data_source, RtspNativeCallBack *callBack);

    ~RtspPlayer();

    void setVideoRenderCallback(VideoRenderCallback videoCallBack);

    void prepare();

    void prepare_();

    void start();

    void start_readPacket();

    void start_video();

    void start_play_video();

    void setWindow(jobject surface);

    void start_audio();

    void restart();

    void stop();

    void release();

    bool isPlaying = true;


private:
    char *data_source = 0;



    AVFormatContext *formatContext = 0;

    AVCodecContext *videoContext;
    AVCodecContext *audioContext;

    RtspNativeCallBack *callBack;



    bool isStop = false;

    int video_index = -1;

    int audio_index = -1;

    SafeQueue<AVPacket *> video_packages; //  视频 的压缩数据包 (是编码的数据包)
    SafeQueue<AVPacket *> audio_packages; // 音频  的压缩数据包 (是编码的数据包)

    SafeQueue<AVFrame *> video_frames; // 视频 的原始数据包（可以直接 渲染 和 播放 的）

    pthread_t pid_video_decode;  // 视频解码线程
    pthread_t pid_video_play;  // 视频播放线程
    pthread_t pid_start;
    pthread_t pid_prepare;

    pthread_t pid_audio_decode;  // 音频解码线程

    ANativeWindow *nativeWindow = 0;

    VideoRenderCallback videoCallback;
};


#endif //ANDFFME_RTSPPLAYER_H
