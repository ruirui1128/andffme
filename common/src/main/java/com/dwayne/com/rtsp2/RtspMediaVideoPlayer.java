package com.dwayne.com.rtsp2;

import android.media.MediaCodec;
import android.media.MediaFormat;
import android.view.Surface;

import java.io.IOException;
import java.nio.ByteBuffer;

public class RtspMediaVideoPlayer {
    private MediaCodec mediaCodec;
    private Surface surface;

    public RtspMediaVideoPlayer(Surface surface) {
        this.surface = surface;
    }


    public void start(String mimeType, int width, int height) {
        try {
            mediaCodec = MediaCodec.createDecoderByType(mimeType);
            MediaFormat format = MediaFormat.createVideoFormat(mimeType, width, height);
            mediaCodec.configure(format, surface, null, 0);
            mediaCodec.start();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    public void feedInputBuffer(byte[] frameData, long presentationTimeUs) {
        ByteBuffer inputBuffer = ByteBuffer.wrap(frameData);
        int inputBufferIndex = mediaCodec.dequeueInputBuffer(10000);
        if (inputBufferIndex >= 0) {
            ByteBuffer buffer = mediaCodec.getInputBuffer(inputBufferIndex);
            buffer.clear();
            buffer.put(inputBuffer);
            mediaCodec.queueInputBuffer(inputBufferIndex, 0, frameData.length, presentationTimeUs, 0);
        }

        MediaCodec.BufferInfo bufferInfo = new MediaCodec.BufferInfo();
        int outputBufferIndex = mediaCodec.dequeueOutputBuffer(bufferInfo, 10000);
        while (outputBufferIndex >= 0) {
            mediaCodec.releaseOutputBuffer(outputBufferIndex, true); // 渲染到Surface
            outputBufferIndex = mediaCodec.dequeueOutputBuffer(bufferInfo, 0);
        }
    }


    public void stop() {
        mediaCodec.stop();
        mediaCodec.release();
    }

}
