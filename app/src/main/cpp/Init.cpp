//
// Created by S on 2024/9/14.
//

// Init.cpp
#include <jni.h>

extern bool Player_OnLoad(JavaVM* vm);
extern bool RtmpPusher_OnLoad(JavaVM* vm);

//JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
//
//    Player_OnLoad(vm);
//    RtmpPusher_OnLoad(vm);
//    return JNI_VERSION_1_6;
//}
