package com.mind.andffme.pull

import android.content.Context
import android.content.Intent
import android.os.Bundle
import android.view.SurfaceHolder
import androidx.appcompat.app.AppCompatActivity
import com.mind.andffme.databinding.ActivityRtmpPull2Binding
import com.mind.andffme.databinding.ActivityRtmpPullBinding
import com.mind.andffme.pull.player.play.Player
import kotlin.concurrent.thread


fun launchRtmp2(context: Context) {
    context.startActivity(Intent(context, RtmpPullActivity2::class.java))
}

class RtmpPullActivity2 : AppCompatActivity() {

    private lateinit var bind: ActivityRtmpPull2Binding

    companion object {
        private val TAG = "player"
        private const val URL = "rtmp://192.168.20.53/live/mystream4"
    }

    private var player: Player? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        bind = ActivityRtmpPull2Binding.inflate(layoutInflater)
        bind.btnStart.setOnClickListener {
            player?.start()
        }
        setContentView(bind.root)
        player = Player()
        player?.setDataSource(URL)
        player?.prepare()
        player?.setSurfaceView(bind.surfaceView)

    }
}