package com.mind.andffme.rtmp

import android.content.Context
import android.content.Intent
import android.os.Bundle
import android.util.Log
import androidx.appcompat.app.AppCompatActivity
import com.dwayne.com.player.play.Player
import com.mind.andffme.databinding.ActivityRtmpPullBinding
import kotlin.concurrent.thread


fun launchRtmpPull(context: Context) {
    context.startActivity(Intent(context, RtmpPullActivity::class.java))
}

class RtmpPullActivity : AppCompatActivity() {

    companion object {
        private const val PULL_URL = "rtmp://192.168.20.53/live/mystream110"
    }

    private lateinit var binding: ActivityRtmpPullBinding

    private var player: Player? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityRtmpPullBinding.inflate(layoutInflater)
        setContentView(binding.root)
        player = Player()
        player?.setDataSource(PULL_URL)
        player?.prepare()
        player?.setSurfaceView(binding.surfaceView)

        binding.btnStart.setOnClickListener {
            player?.start()
        }
        binding.btnStop.setOnClickListener {
            player?.stop()
        }

        binding.btnRelease.setOnClickListener {
            Log.e("tag","===点击销毁========")
            player?.release()
        }

    }

    override fun onStop() {
        super.onStop()
        player?.stop()
    }

    override fun onDestroy() {
        super.onDestroy()
        player?.release()
    }
}