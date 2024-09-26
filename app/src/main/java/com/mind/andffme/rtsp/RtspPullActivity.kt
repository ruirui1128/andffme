package com.mind.andffme.rtsp

import android.content.Context
import android.content.Intent
import android.media.AudioFormat
import android.media.AudioManager
import android.media.AudioTrack
import android.media.MediaCodec
import android.media.MediaFormat
import android.os.Bundle
import android.util.Log
import android.view.SurfaceHolder
import androidx.appcompat.app.AppCompatActivity
import com.dwayne.com.rtsp.RtspPull
import com.dwayne.com.rtsp.RtspPullListener
import com.mind.andffme.databinding.ActivityRtspPullBinding
import java.io.IOException
import kotlin.concurrent.thread


fun launchRtspActivity(activity: Context?) {
    activity?.startActivity(Intent(activity, RtspPullActivity::class.java))
}

class RtspPullActivity : AppCompatActivity(), SurfaceHolder.Callback {

    companion object {
        private const val RTMP_URL = "rtmp://192.168.20.53/live/mystream110"
        private const val RTSP_URL =
            "rtsp://admin:a1234567@192.168.22.119:554/h264/ch1/sub/av_stream"
    }

    private lateinit var bind: ActivityRtspPullBinding
    private lateinit var rtspPull: RtspPull

    private lateinit var surfaceHolder: SurfaceHolder
    private var videoCodec: MediaCodec? = null
    private lateinit var audioTrack: AudioTrack
    private var isDecoderInitialized = false

    private var isPaused = false

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        bind = ActivityRtspPullBinding.inflate(layoutInflater)
        setContentView(bind.root)
        rtspPull = RtspPull()
        surfaceHolder = bind.surfaceView.holder
        initView()
    }

    private fun initView() {
        bind.btnPull.setOnClickListener {
            rtspPull.initPrepeare(RTSP_URL)
        }
        bind.btnStart.setOnClickListener {
            thread {
                rtspPull.startRtsp()
            }
        }

        bind.btnRelease.setOnClickListener {
            thread {
                rtspPull.releaseRtsp()
            }
        }

        bind.surfaceView.holder.addCallback(this)
    }



    override fun onDestroy() {
        super.onDestroy()
        thread {
            rtspPull.releaseRtsp()
        }
    }

    override fun surfaceCreated(holder: SurfaceHolder) {
        rtspPull.setSurfaceNative(holder.surface)
    }

    override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {

    }

    override fun surfaceDestroyed(holder: SurfaceHolder) {

    }

}