package com.dwayne.com.rtsp;

import android.util.Log;
import android.view.Surface;

public class RtspPull {
    static {
        System.loadLibrary("rtsp"); //加载推流 so
    }

    public void onVideoFrame(byte[] data, int length, int width, int height) {

    }

    public void onAudioFrame(byte[] data, int length) {

    }

    public void onVideo(byte[] data, int length, int width, int height) {

    }

    public void onAudio(byte[] data, int length) {

    }


    public void onAction(int action) {
        Log.e("RTSP", "onAction:" + action);
    }


    // 初始化 JNI 层的 RTSP 拉流，传入 RTSP 流的 URL
    public native void initRTSPStream(String url);

    // 拉取视频帧数据
    public native void pullVideoData();

    // 拉取音频帧数据
    public native void pullAudioData();

    // 停止 RTSP 流的拉取
    public native void stopRTSPStream();

    // 暂停 RTSP 拉流
    public native void pauseRTSPStream();

    // 重新开始 RTSP 拉流
    public native void resumeRTSPStream();

    //
    public native void initPrepeare(String url);

    public native void setSurfaceNative(Surface surface);

    public native void startRtsp();

    public native void releaseRtsp();
}
