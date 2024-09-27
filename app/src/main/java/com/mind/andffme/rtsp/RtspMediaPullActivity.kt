package com.mind.andffme.rtsp

import android.content.Context
import android.content.Intent
import android.os.Bundle
import android.view.View
import androidx.appcompat.app.AppCompatActivity
import com.dwayne.com.rtsp2.RtspMediaPull
import com.mind.andffme.databinding.ActivityRtspMediaPullBinding


fun launcherRtspMediaPull(ctx: Context) {
    ctx.startActivity(Intent(ctx, RtspMediaPullActivity::class.java))
}

class RtspMediaPullActivity : AppCompatActivity() {

    companion object {
        private const val RTMP_URL = "rtmp://192.168.20.53/live/mystream110"
        private const val RTSP_URL =
            "rtsp://admin:a1234567@192.168.22.119:554/h264/ch1/sub/av_stream"
    }


    private lateinit var bind: ActivityRtspMediaPullBinding
    private lateinit var rtmpPuller: RtspMediaPull

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        bind = ActivityRtspMediaPullBinding.inflate(layoutInflater)
        setContentView(bind.root)
        rtmpPuller = RtspMediaPull()
        bind.btnPull.setOnClickListener {
            rtmpPuller.initPrepeareM(RTSP_URL)
        }
        bind.btnStart.setOnClickListener {
            rtmpPuller.startRtsp2()
        }
        bind.surfaceView.post {
            rtmpPuller.initMedia(bind.surfaceView.holder.surface)
        }
    }


}