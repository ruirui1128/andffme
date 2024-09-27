#include <jni.h>
#include <android/log.h>
#include <unistd.h>  // 用于 sleep 函数
#include "safe_queue.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/time.h>
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
}


AVFormatContext *fmt_ctx = nullptr; //  RTSP流上下文
AVCodecContext *video_codec_ctx = nullptr; // 视频编码器上下文
AVCodecContext *audio_codec_ctx = nullptr; // 音频编码器上下文

jobject instance;

int video_stream_index = -1; // 视频索引
int audio_stream_index = -1; // 音频索引

pthread_t pid_read_packet;  // 读取数据线程
pthread_t pid_decode_video; // 视频解码线程
SafeQueue<AVPacket *> video_packages; //  视频 的压缩数据包 (是编码的数据包)


#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "FFmpegRTSP", __VA_ARGS__)

bool isPlaying = true;


void videoDecoder(JNIEnv *env) {
    AVPacket *packet = av_packet_alloc(); // 正确分配 AVPacket
    if (!packet) {
        LOGE("无法分配 AVPacket");
        return;
    }

    jclass cls = env->GetObjectClass(instance); // 移到循环外部
    jmethodID methodID = env->GetMethodID(cls, "onFrameReceived", "([BII)V"); // 移到循环外部
    LOGE("==============开始视频解码==============");
    while (isPlaying) {
        int ret = video_packages.pop(packet);
        if (!isPlaying) {
            LOGE("停止播放 isPlaying %d", isPlaying);
            break;
        }
        if (!ret) {
            continue;
        }

        // 解码
        ret = avcodec_send_packet(video_codec_ctx, packet);
        if (ret < 0) {
            LOGE("avcodec_send_packet 失败，ret %d", ret);
            continue; // 处理错误并继续循环
        }

        AVFrame *frame = av_frame_alloc();
        if (!frame) {
            LOGE("无法分配 AVFrame");
            continue; // 处理错误并继续循环
        }

        ret = avcodec_receive_frame(video_codec_ctx, frame);
        if (ret == AVERROR(EAGAIN)) {
            av_frame_free(&frame);
            continue;
        } else if (ret < 0) {
            LOGE("avcodec_receive_frame 失败，ret %d", ret);
            av_frame_free(&frame);
            break;
        }

        LOGE("QUEUE SIZE: %d", video_packages.queueSize());

        // 创建Java字节数组以传递给Java层
        jbyteArray frameData = env->NewByteArray(frame->linesize[0]);
        if (frameData) {

            env->SetByteArrayRegion(frameData, 0, frame->linesize[0],
                                    reinterpret_cast<const jbyte *>(frame->data[0]));
            env->CallVoidMethod(instance, methodID, frameData, video_codec_ctx->width, video_codec_ctx->height);

            env->DeleteLocalRef(frameData);
        }

        av_frame_free(&frame);
    }
    env->DeleteLocalRef(cls);
    av_packet_free(&packet); // 确保释放 packet
}



JavaVM *globalJvm = 0;

int JNI_OnLoad(JavaVM *javaVM1, void *pVoid) {
    ::globalJvm = javaVM1;
    return JNI_VERSION_1_6;
}


void *decoderVideoThread(void *pVoid) {
    // 获取 JNI 环境

    JNIEnv *env;
    if (globalJvm->AttachCurrentThread(&env, nullptr) != JNI_OK) {
        LOGE("Failed to attach current thread to JVM");
        return nullptr;
    }

    jint ret = globalJvm->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (ret != JNI_OK) {
        LOGE("Failed to get JNI environment");
        return nullptr;
    }
    videoDecoder(env);
    return 0;
}


void readPacket() {
    video_packages.setFlag(1);
    while (isPlaying) {
        if (!fmt_ctx) {
            LOGE("formatContext formatContext 已经被释放了");
            return;
        }
        AVPacket *packet = av_packet_alloc();
        int ret = av_read_frame(fmt_ctx, packet);
        if (!ret) {
            if (video_stream_index == packet->stream_index) {
                video_packages.push(packet);
            } else if (audio_stream_index == packet->stream_index) {
                // audio_packages.push(packet);
            }
        } else if (ret == AVERROR_EOF) {
            // 读取完成
            LOGE("stream_index 拆包完成 %s", "读取完成了");
            isPlaying = false;
            av_packet_free(&packet);
            break;
        } else {
            LOGE("stream_index 拆包 %s", "读取失败");
            av_packet_free(&packet);
            break;//读取失败
        }
    }

}

void *startReadPacket(void *pVoid) {
    readPacket();
    return 0;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_dwayne_com_rtsp2_RtspMediaPull_initPrepeareM(JNIEnv *env, jobject obj, jstring url) {

    const char *rtsp_url = env->GetStringUTFChars(url, nullptr);
    fmt_ctx = avformat_alloc_context();

    avformat_network_init();  // 初始化网络组件

    // 打卡steam流
    if (avformat_open_input(&fmt_ctx, rtsp_url, nullptr, nullptr)) {
        LOGE("Failed to open RTSP stream");
        return;
    }
    // 获取流信息
    if (avformat_find_stream_info(fmt_ctx, nullptr) < 0) {
        LOGE("Failed to find stream info");
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


    env->ReleaseStringUTFChars(url, rtsp_url);  // 释放 URL 字符串

    LOGE("========初始化成功=======");


//    for (int i = 0; i < 100; ++i) {
//        env->CallVoidMethod(instance, env->GetMethodID(env->GetObjectClass(instance), "onFrameReceived", "(I)V"), i);
//    }

}
extern "C"
JNIEXPORT void JNICALL
Java_com_dwayne_com_rtsp2_RtspMediaPull_startRtsp2(JNIEnv *env, jobject thiz) {
    instance = env->NewGlobalRef(thiz);
    pthread_create(&pid_read_packet, 0, startReadPacket, nullptr);
    pthread_create(&pid_decode_video, 0, decoderVideoThread, nullptr);

}