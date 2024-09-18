package com.mind.andffme.pull;

import android.media.MediaCodec;
import android.media.MediaFormat;
import android.util.Log;
import android.view.Surface;

import java.nio.ByteBuffer;

public class RtmpPullStreamPlayer {
    static {
        System.loadLibrary("andffme"); // 加载 JNI 库
    }

    private static final String URL = "rtmp://192.168.20.53/live/mystream4";
    private static final String URL2 = "rtmp://192.168.20.53/live/mystream4";

    // 本地方法，拉取 RTMP 流
    public native void pullRTMPStream2(String url, Surface surface);


    // 启动流拉取
    public void startStreaming(Surface surface) {
        pullRTMPStream2(URL,surface);
    }


}
