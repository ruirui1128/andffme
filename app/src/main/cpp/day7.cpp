#include <jni.h>
#include <string>
#include <malloc.h>

extern "C" {
#include "include/libavutil/avutil.h"
}

extern "C" {
#include "include/libavformat//avformat.h"
}


#include <android/log.h>

int __android_log_print(int priority, const char *tag, const char *fmt, ...);

#define LOG_TAG "NativeCode"

#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)


static char *jstringToChar(JNIEnv *env, jstring jstr) {
    char *rtn = NULL;
    jclass clsstring = env->FindClass("java/lang/String");
    jstring strencode = env->NewStringUTF("utf-8");
    jmethodID mid = env->GetMethodID(clsstring, "getBytes", "(Ljava/lang/String;)[B");
    jbyteArray barr = (jbyteArray) env->CallObjectMethod(jstr, mid, strencode);
    jsize alen = env->GetArrayLength(barr);
    jbyte *ba = env->GetByteArrayElements(barr, JNI_FALSE);

    if (alen > 0) {
        rtn = new char[alen + 1];
        memcpy(rtn, ba, alen);
        rtn[alen] = 0;
    }
    env->ReleaseByteArrayElements(barr, ba, 0);
    return rtn;
}

void releaseChars(char *str) {
    if (str != nullptr) {
        free(str); // 释放内存
    }
}


extern "C"
JNIEXPORT jstring JNICALL
Java_com_mind_andffme_day_DayPresenter_stringFromJNI(JNIEnv *env, jobject thiz) {
    std::string hello = "Hello from C++";
    hello += av_version_info();
    LOGD("==========%s==========", hello.c_str());
    return env->NewStringUTF(hello.c_str());
}
extern "C"
JNIEXPORT void JNICALL
Java_com_mind_andffme_day_DayPresenter_printVideoInfo(JNIEnv *env, jobject thiz, jstring path) {
    const char *in_file_path = jstringToChar(env, path);
    LOGD("========printVideoInfo filePath: %s", in_file_path);

    AVFormatContext *ifmt_ctx = nullptr;  // 媒体文件格式上下文
    int ret = avformat_open_input(&ifmt_ctx, in_file_path, NULL, NULL);

    if (ret < 0) {  // 打开媒体文件失败
        char buf[1024] = {0};
        av_strerror(ret, buf, sizeof(buf) - 1);
        LOGE("========avformat_open_input error: %s", buf);
        // 清理资源
        if (ifmt_ctx != nullptr) avformat_close_input(&ifmt_ctx);
        if (in_file_path != nullptr) releaseChars((char *) in_file_path);
        return;
    }

    // 获取流媒体的信息
    ret = avformat_find_stream_info(ifmt_ctx, NULL);
    if (ret < 0) {  // 获取流信息失败
        char buf[1024] = {0};
        av_strerror(ret, buf, sizeof(buf) - 1);
        LOGE("========avformat_find_stream_info error: %s", buf);
        // 清理资源
        if (ifmt_ctx != nullptr) avformat_close_input(&ifmt_ctx);
        if (in_file_path != nullptr) releaseChars((char *) in_file_path);
        return;
    }

    LOGD("==============媒体文件打开成功======================");
    av_dump_format(ifmt_ctx, 0, in_file_path, 0);

    // 获取视频时长
    int total_seconds, hours, minutes, seconds;
    total_seconds = (ifmt_ctx->duration) / AV_TIME_BASE;
    hours = total_seconds / 3600;
    minutes = (total_seconds % 3600) / 60;
    seconds = (total_seconds % 60);

    LOGD("====视频时长: %02d:%02d:%02d", hours, minutes, seconds);

    // 清理资源
    if (ifmt_ctx != nullptr) avformat_close_input(&ifmt_ctx);
    if (in_file_path != nullptr) releaseChars((char *) in_file_path);

}