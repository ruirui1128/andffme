package com.mind.andffme.push.stream;

import android.view.SurfaceHolder;


public abstract class VideoStreamBase {

    public abstract void startLive();

    public abstract void setPreviewDisplay(SurfaceHolder surfaceHolder);

    public abstract void switchCamera();

    public abstract void stopLive();

    public abstract void release();

    public abstract void onPreviewDegreeChanged(int degree);

}
