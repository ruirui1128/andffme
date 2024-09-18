package com.mind.andffme.pull

import android.content.Context
import android.content.Intent
import android.os.Bundle
import android.view.SurfaceHolder
import androidx.appcompat.app.AppCompatActivity
import com.mind.andffme.databinding.ActivityRtmpPullBinding
import kotlin.concurrent.thread


fun launchRtmp(context: Context) {
    context.startActivity(Intent(context, RtmpPullActivity::class.java))
}

class RtmpPullActivity : AppCompatActivity() {

    private lateinit var bind: ActivityRtmpPullBinding
    private var streamPlayer: RtmpPullStreamPlayer? = null
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        bind = ActivityRtmpPullBinding.inflate(layoutInflater)
        setContentView(bind.root)
        bind.surfaceView.holder.addCallback(object : SurfaceHolder.Callback {
            override fun surfaceCreated(holder: SurfaceHolder) {
                thread {
                    streamPlayer = RtmpPullStreamPlayer()
                    streamPlayer?.startStreaming(holder.surface)
                }
            }

            override fun surfaceChanged(
                holder: SurfaceHolder,
                format: Int,
                width: Int,
                height: Int
            ) {
            }

            override fun surfaceDestroyed(holder: SurfaceHolder) {
                // 清理资源
                if (streamPlayer != null) {
                    streamPlayer = null
                }
            }
        })
    }
}