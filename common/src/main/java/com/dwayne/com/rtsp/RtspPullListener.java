package com.dwayne.com.rtsp;

public interface RtspPullListener {
    public void onVideoFrame(byte[] data, int length,int width, int height);

    public void onAudioFrame(byte[] data, int length);
}
