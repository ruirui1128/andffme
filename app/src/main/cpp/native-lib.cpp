#include <jni.h>
#include <string>
#include <android/log.h>

int __android_log_print(int priority, const char *tag, const char *fmt, ...);

#define LOG_TAG "NativeCode"

#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

extern "C" {
#include "include/libavutil/avutil.h"
}

extern "C"
JNIEXPORT jstring JNICALL
Java_com_mind_andffme_AndFFmeHelper_stringFromJNI(JNIEnv *env, jobject thiz) {
    std::string hello = "Hello from C++";
    hello += av_version_info();
    LOGD("==========%s==========", hello.c_str());
    return env->NewStringUTF(hello.c_str());
}