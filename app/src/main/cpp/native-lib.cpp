#include <jni.h>
#include <string>
#include <android/log.h>



extern "C" {
#include "libavutil/avutil.h"
}

extern "C"
JNIEXPORT jstring JNICALL
Java_com_mind_andffme_AndFFmeHelper_stringFromJNI(JNIEnv *env, jobject thiz) {
    std::string hello = "Hello from C++";
    hello += av_version_info();
    return env->NewStringUTF(hello.c_str());
}