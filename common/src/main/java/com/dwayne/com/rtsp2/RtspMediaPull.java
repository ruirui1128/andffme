package com.dwayne.com.rtsp2;

import android.util.Log;
import android.view.Surface;

public class RtspMediaPull {
    static {
        System.loadLibrary("rtspm"); //加载推流 so
    }

    private RtspMediaVideoPlayer player;

    public native void initPrepeareM(String url);

    public native void startRtsp2();


    public void initMedia(Surface surface) {
        player = new RtspMediaVideoPlayer(surface);
        String mimeType = "video/avc"; // H.264
        int width = 1280; // 视频宽度
        int height = 720; // 视频高度
        player.start(mimeType, width, height);
    }


    public void stopMedia(){
        if (player != null){
            player.stop();
        }
    }

    public void onFrameReceived(byte[] frameData, int width, int height) {
       if ( player!=null){
           player.feedInputBuffer(frameData,System.nanoTime() / 1000);
       }
    }


}
