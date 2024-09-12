//
// Created by S on 2024/9/12.
//
#include <jni.h>
#include <string>
#include "PacketQueue.h"
#include "PushInterface.h"
#include "VideoStream.h"
#include "AudioStream.h"





PacketQueue<RTMPPacket *> packets;
VideoStream *videoStream = nullptr;
AudioStream *audioStream = nullptr;

std::atomic<bool> isPushing;
uint32_t start_time;

//use to get thread's JNIEnv
JavaVM *javaVM;
//callback object
jobject jobject_error;

//when calling System.loadLibrary, will callback it
jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    javaVM = vm;
    return JNI_VERSION_1_6;
}

//callback error to java
void throwErrToJava(int error_code) {
    JNIEnv *env;
    javaVM->AttachCurrentThread(&env, nullptr);
    jclass classErr = env->GetObjectClass(jobject_error);
    jmethodID methodErr = env->GetMethodID(classErr, "errorFromNative", "(I)V");
    env->CallVoidMethod(jobject_error, methodErr, error_code);
    javaVM->DetachCurrentThread();
}

void callback(RTMPPacket *packet) {
    if (packet) {
        packet->m_nTimeStamp = RTMP_GetTime() - start_time;
        packets.push(packet);
    }
}

void releasePackets(RTMPPacket *&packet) {
    if (packet) {
        RTMPPacket_Free(packet);
        delete packet;
        packet = nullptr;
    }
}


void *start(void *args) {
//    char *url = static_cast<char *>(args);
//    RTMP *rtmp;
//    do {
//        rtmp = RTMP_Alloc(); // 分配一个RTMP结构体
//        if (!rtmp) {  // 检测是否分配成功
//            break;
//        }
//        RTMP_Init(rtmp);  // 初始化结构体
//        int ret = RTMP_SetupURL(rtmp, url); // 设置推流的url
//        if (!ret) { // 检测是否分配成功
//            break;
//        }
//        rtmp->Link.timeout = 10; // 设置连接时间
//        RTMP_EnableWrite(rtmp);  // 写入模式
//        ret = RTMP_Connect(rtmp, nullptr); // 连接服务器
//        if (!ret) {  // 检查是否连接失败
//            throwErrToJava(ERROR_RTMP_CONNECT);  // 抛出异常
//            break;
//        }
//        ret = RTMP_ConnectStream(rtmp, 0); // 连接流
//        if (!ret) {
//            throwErrToJava(ERROR_RTMP_CONNECT_STREAM); // 抛出异常
//            break;
//        }
//        start_time = RTMP_GetTime(); // 开始记录时间
//        isPushing = true;  // 更新推流标记
//        packets.setRunning(true);
//        callback(audioStream->getAudioTag());  // 设置音频流
//        RTMPPacket *packet = nullptr;
//        while (isPushing) {
//            packets.pop(packet);//  从队列中获取一个数据包
//            if (!isPushing) {  // 如果停止推流，退出循环
//                break;
//            }
//            if (!packet) {  // 如果没有获取到包，继续
//                continue;
//            }
//            packet->m_nInfoField2 = rtmp->m_stream_id;  // 设置RTMP包的流ID
//            ret = RTMP_SendPacket(rtmp, packet, 1); // 发送数据包
//            releasePackets(packet); // 是否数据包
//            if (!ret) {  // 如果发送失败，抛出错误并退出
//                //LOGE("RTMP_SendPacket fail...");
//                throwErrToJava(ERROR_RTMP_SEND_PACKET);
//                break;
//            }
//        }
//        releasePackets(packet);  // 释放最后一个包
//
//    } while (0);
//
//    isPushing = false;  // 停止推流
//    packets.setRunning(false);  // 停止数据包队列
//    packets.clear(); // 清空数据包队列
//    if (rtmp) { // 关闭 RTMP 连接并释放 RTMP 资源
//        RTMP_Close(rtmp);
//        RTMP_Free(rtmp);
//    }
//    delete (url);  // 释放 URL 的内存
//    return nullptr;

    char *url = static_cast<char *>(args);
    RTMP *rtmp;
    do {
        rtmp = RTMP_Alloc();
        if (!rtmp) {
            LOGE("RTMP_Alloc fail");
            break;
        }
        RTMP_Init(rtmp);
        int ret = RTMP_SetupURL(rtmp, url);
        if (!ret) {
            LOGE("RTMP_SetupURL:%s", url);
            break;
        }
        //timeout
        rtmp->Link.timeout = 5;
        RTMP_EnableWrite(rtmp);
        ret = RTMP_Connect(rtmp, nullptr);
        if (!ret) {
            LOGE("RTMP_Connect:%s", url);
            throwErrToJava(ERROR_RTMP_CONNECT);
            break;
        }
        ret = RTMP_ConnectStream(rtmp, 0);
        if (!ret) {
            LOGE("RTMP_ConnectStream:%s", url);
            throwErrToJava(ERROR_RTMP_CONNECT_STREAM);
            break;
        }
        //start time
        start_time = RTMP_GetTime();
        //start pushing
        isPushing = true;
        packets.setRunning(true);
        callback(audioStream->getAudioTag());
        RTMPPacket *packet = nullptr;
        while (isPushing) {
            packets.pop(packet);
            if (!isPushing) {
                break;
            }
            if (!packet) {
                continue;
            }

            if (!RTMP_IsConnected(rtmp)) {
                LOGE("RTMP connection lost...");
                break;
            }
            packet->m_nInfoField2 = rtmp->m_stream_id;
            ret = RTMP_SendPacket(rtmp, packet, 1);
            releasePackets(packet);
            if (!ret) {
                LOGE("RTMP_SendPacket fail...");
                throwErrToJava(ERROR_RTMP_SEND_PACKET);
                break;
            }
        }
        releasePackets(packet);
    } while (0);
    isPushing = false;
    packets.setRunning(false);
    packets.clear();
    if (rtmp) {
        RTMP_Close(rtmp);
        RTMP_Free(rtmp);
    }
    delete (url);
    return nullptr;

}







/**
 * push 初始化
 */
extern "C"
JNIEXPORT void JNICALL
Java_com_mind_andffme_push_Pusher_native_1init(JNIEnv *env, jobject thiz) {
    videoStream = new VideoStream();
    videoStream->setVideoCallback(callback);
    audioStream = new AudioStream();
    audioStream->setAudioCallback(callback);
    packets.setReleaseCallback(releasePackets);
    jobject_error = env->NewGlobalRef(thiz);
}

/**
 *push 开启一个线程 来发送数据包
 */
extern "C"
JNIEXPORT void JNICALL
Java_com_mind_andffme_push_Pusher_native_1start(JNIEnv *env, jobject thiz, jstring _path) {
    if (isPushing) {
        return;
    }
    const char *path = env->GetStringUTFChars(_path, nullptr);
    char *url = new char[strlen(path) + 1];  // 动态分配内存
    stpcpy(url, path);  // 复制url
    env->ReleaseStringUTFChars(_path, path);  // 释放由 `GetStringUTFChars` 分配的内存

    std::thread pushThread(start, url); //创建一个线程
    pushThread.detach();  // 将线程与主线程分离 独立运行

}

/**
 * 将视频的编码参数由java层传入c层 （分辨率，帧率，码率）
 */
extern "C"
JNIEXPORT void JNICALL
Java_com_mind_andffme_push_Pusher_native_1setVideoCodecInfo(JNIEnv *env, jobject thiz, jint width,
                                                            jint height, jint fps, jint bitrate) {
    if (videoStream) {
        int ret = videoStream->setVideoEncInfo(width, height, fps, bitrate);
        if (ret < 0) {
            throwErrToJava(ERROR_VIDEO_ENCODER_OPEN);
        }
    }
}

/**
 * 将音频的参数由java层传入c层 （采样率 通道数）
 */
extern "C"
JNIEXPORT void JNICALL
Java_com_mind_andffme_push_Pusher_native_1setAudioCodecInfo(JNIEnv *env, jobject thiz,
                                                            jint sample_rate_in_hz, jint channels) {
    if (audioStream) {
        int ret = audioStream->setAudioEncInfo(sample_rate_in_hz, channels);
        if (ret < 0) {
            throwErrToJava(ERROR_AUDIO_ENCODER_OPEN);
        }
    }
}

/**
 * 获取音频的采样数
 */
extern "C"
JNIEXPORT jint JNICALL
Java_com_mind_andffme_push_Pusher_native_1getInputSamples(JNIEnv *env, jobject thiz) {
    if (audioStream) {
        return audioStream->getInputSamples();
    }
    return -1;
}

/**
 * 推送音频数据
 */
extern "C"
JNIEXPORT void JNICALL
Java_com_mind_andffme_push_Pusher_native_1pushAudio(JNIEnv *env, jobject thiz, jbyteArray data_) {
    if (!audioStream || !isPushing) {
        return;
    }
    jbyte *data = env->GetByteArrayElements(data_, nullptr);
    audioStream->encodeData(data);
    env->ReleaseByteArrayElements(data_, data, 0);

}
/**
 * 推流音频数据
 */
extern "C"
JNIEXPORT void JNICALL
Java_com_mind_andffme_push_Pusher_native_1pushVideo(JNIEnv *env, jobject thiz, jbyteArray yuv,
                                                    jint camera_type) {
    if (!videoStream || !isPushing) {
        return;
    }
    jbyte *yuv_plane = env->GetByteArrayElements(yuv, JNI_FALSE);
    videoStream->encodeVideo(yuv_plane, camera_type);
    env->ReleaseByteArrayElements(yuv, yuv_plane, 0);

}
extern "C"
JNIEXPORT void JNICALL
Java_com_mind_andffme_push_Pusher_native_1stop(JNIEnv *env, jobject thiz) {
    isPushing = false;
    packets.setRunning(false);
}
extern "C"
JNIEXPORT void JNICALL
Java_com_mind_andffme_push_Pusher_native_1release(JNIEnv *env, jobject thiz) {
    env->DeleteGlobalRef(jobject_error);
    delete videoStream;
    videoStream = nullptr;
    delete audioStream;
    audioStream = nullptr;
}