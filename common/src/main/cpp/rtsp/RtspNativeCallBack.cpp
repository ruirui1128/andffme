//
// Created by S on 2024/9/25.
//
#include "include/RtspNativeCallBack.h"

RtspNativeCallBack::RtspNativeCallBack(JavaVM *javaVM, JNIEnv *env, jobject instance) {
    this->javaVM = javaVM;
    this->env = env;
    this->instance = env->NewGlobalRef(instance);  //jobject一旦涉及到跨函数，跨线程，必须是全局引用）

    //拿到 Java 对象的 class
    jclass mPlayClass = env->GetObjectClass(this->instance);

    const char *sigErr = "(I)V"; //Int 参数，无返回值

    //"onVideoFrame",
    const char *signVideo = "([BIII)V";
    // onAudioFrame
    const char *signAudio = "([BI)V";

    this->jmd_error = env->GetMethodID(mPlayClass, "onAction", sigErr);
    this->jmd_video = env->GetMethodID(mPlayClass, "onVideo", signVideo);
    this->jmd_audio = env->GetMethodID(mPlayClass, "onAudio", signAudio);


}

void RtspNativeCallBack::onAction(int thread_mode, int error_code) {
    if (thread_mode == THREAD_MAIN) {
        env->CallVoidMethod(this->instance, jmd_error, error_code);//主线程可以直接调用 Java 方法
    } else {
        // 附加native 线程到JVM 的方式
        JNIEnv *jniEnv = nullptr;
        jint ret = javaVM->AttachCurrentThread(&jniEnv, 0);
        if (ret != JNI_OK) {
            return;
        }
        jniEnv->CallVoidMethod(this->instance, jmd_error, error_code);
        javaVM->DetachCurrentThread();//解除附加
    }

}


void
RtspNativeCallBack::onVideo(int thread_mode, jbyte *data, int length, int width, int height) {

    jint ret = javaVM->AttachCurrentThread(&env, 0);
    if (ret != JNI_OK) {
        return;
    }
    jbyteArray video_data = env->NewByteArray(length);  // 创建 Java 层的 byte 数组
    env->SetByteArrayRegion(video_data, 0, length, data);  // 将视频帧数据传递给 Java 层
    env->CallVoidMethod(this->instance, jmd_video, video_data, length, width, height);
    env->DeleteLocalRef(video_data);  // 释放局部引用

}


void RtspNativeCallBack::onAudio(int thread_mode, jbyteArray data, int length) {
    if (thread_mode == THREAD_MAIN) {
        env->CallVoidMethod(this->instance, jmd_audio, data, length);//主线程可以直接调用 Java 方法
    } else {
        // 附加native 线程到JVM 的方式
        JNIEnv *jniEnv = nullptr;
        jint ret = javaVM->AttachCurrentThread(&jniEnv, 0);
        if (ret != JNI_OK) {
            return;
        }
        env->CallVoidMethod(this->instance, jmd_audio, data, length);//主线程可以直接调用 Java 方法
        javaVM->DetachCurrentThread();//解除附加
    }

}


/**
 * 析构函数：专门完成释放的工作
 */
RtspNativeCallBack::~RtspNativeCallBack() {
    LOGD("~JNICallback")
    this->javaVM = 0;
    env->DeleteGlobalRef(this->instance);//释放全局
    this->instance = 0;
    env = 0;

}

