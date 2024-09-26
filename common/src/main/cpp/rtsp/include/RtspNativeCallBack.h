//
// Created by S on 2024/9/25.
//

#ifndef ANDFFME_RTSPNATIVECALLBACK_H
#define ANDFFME_RTSPNATIVECALLBACK_H

#include <jni.h>
#include "Constants.h"

class RtspNativeCallBack {

public:
    RtspNativeCallBack(JavaVM *javaVM, JNIEnv *env, jobject instance);

    // 回调
    void onAction(int thread_mode, int error_code);

    // byte[] data, int length,int width, int height
    void onVideo(int thread_mode, jbyte *data, int length,  int width, int height);

    void onAudio(int thread_mode,jbyteArray data, int length);

    //析构函数声明
    ~RtspNativeCallBack();

private:
    JavaVM *javaVM;
    JNIEnv *env;
    jobject instance;

    jmethodID jmd_error;
    jmethodID jmd_video;
    jmethodID jmd_audio;

};

#endif //ANDFFME_RTSPNATIVECALLBACK_H
