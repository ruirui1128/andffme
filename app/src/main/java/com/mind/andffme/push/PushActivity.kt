package com.mind.andffme.push

import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.media.AudioFormat
import android.net.ConnectivityManager
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.os.Message
import android.text.TextUtils
import android.util.Log
import android.view.SurfaceHolder
import android.view.SurfaceView
import android.view.View
import android.widget.CompoundButton
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import com.mind.andffme.R
import com.mind.andffme.databinding.ActivityLiveBinding
import com.mind.andffme.handler.ConnectionReceiver
import com.mind.andffme.handler.OrientationHandler
import com.mind.andffme.listener.OnNetworkChangeListener
import com.mind.andffme.push.LiveActivity.Companion
import com.mind.andffme.push.camera.Camera2Helper
import com.mind.andffme.push.camera.CameraType
import com.mind.andffme.push.listener.LiveStateChangeListener
import com.mind.andffme.push.param.AudioParam
import com.mind.andffme.push.param.VideoParam


fun launchPush(context: Context) {
    context.startActivity(Intent(context, PushActivity::class.java))
}

class PushActivity : AppCompatActivity(), CompoundButton.OnCheckedChangeListener,
    LiveStateChangeListener, OnNetworkChangeListener, View.OnClickListener {
    companion object {
        private const val LIVE_URL = "rtmp://192.168.20.53/live/mystream4"
        private const val MSG_ERROR = 100
    }

    private var pusher: Pusher? = null
    private var isPushing = false
    private var connectionReceiver: ConnectionReceiver? = null
    private var orientationHandler: OrientationHandler? = null


    private val mHandler = object : Handler(Looper.getMainLooper()) {
        override fun handleMessage(msg: Message) {
            super.handleMessage(msg)
            if (msg.what == MSG_ERROR) {
                val errMsg = msg.obj as String
                if (!TextUtils.isEmpty(errMsg)) {
                    Toast.makeText(this@PushActivity, errMsg, Toast.LENGTH_SHORT).show()
                }
            }
        }
    }


    private lateinit var bind: ActivityLiveBinding
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        bind = ActivityLiveBinding.inflate(layoutInflater)
        setContentView(bind.root)
        initView()
        initPusher()
        registerBroadcast(this)
        orientationHandler = OrientationHandler(this)
        orientationHandler?.enable()
        orientationHandler?.setOnOrientationListener(object :
            OrientationHandler.OnOrientationListener {
            override fun onOrientation(orientation: Int) {
                val previewDegree = (orientation + 90) % 360
                Log.d("previewDegree", "====变化角度:"+previewDegree);
                pusher?.setPreviewDegree(previewDegree)
            }
        })


    }

    private fun initView() {
        bind.btnLive.setOnCheckedChangeListener(this)
        bind.btnMute.setOnCheckedChangeListener(this)
        bind.btnSwap.setOnClickListener {
            pusher?.switchCamera()
        }
    }


    //    private fun registerBroadcast(networkChangeListener: OnNetworkChangeListener) {
//        connectionReceiver = ConnectionReceiver(networkChangeListener)
//        val intentFilter = IntentFilter()
//        intentFilter.addAction(ConnectivityManager.CONNECTIVITY_ACTION)
//        registerReceiver(connectionReceiver, intentFilter)
//    }
    private fun registerBroadcast(networkChangeListener: OnNetworkChangeListener) {
        connectionReceiver = ConnectionReceiver(networkChangeListener)
        val intentFilter = IntentFilter()
        intentFilter.addAction(ConnectivityManager.CONNECTIVITY_ACTION)
        registerReceiver(connectionReceiver, intentFilter)
    }


    private fun initPusher() {
//        val width = 640
//        val height = 480
//        val videoBitRate = 800000 // kb/s
//        val videoFrameRate = 10 // fps
//        val videoParam = VideoParam(
//            width, height,
//            Integer.valueOf(Camera2Helper.CAMERA_ID_BACK), videoBitRate, videoFrameRate
//        )
//        val sampleRate = 44100
//        val channelConfig = AudioFormat.CHANNEL_IN_STEREO
//        val audioFormat = AudioFormat.ENCODING_PCM_16BIT
//        val numChannels = 2
//        val audioParam = AudioParam(sampleRate, channelConfig, audioFormat, numChannels)
//
//        pusher = Pusher(this, videoParam, audioParam, bind.surfaceView, CameraType.CAMERA2)
//        if (bind.surfaceView is SurfaceView) {
//            val holder: SurfaceHolder = (bind.surfaceView as SurfaceView).holder
//            pusher?.setPreviewDisplay(holder)
//        }


        val width = 640
        val height = 480
        val videoBitRate = 800000 // kb/s
        val videoFrameRate = 10 // fps
        val videoParam = VideoParam(
            width, height,
            Integer.valueOf(Camera2Helper.CAMERA_ID_BACK), videoBitRate, videoFrameRate
        )
        val sampleRate = 44100
        val channelConfig = AudioFormat.CHANNEL_IN_STEREO
        val audioFormat = AudioFormat.ENCODING_PCM_16BIT
        val numChannels = 2
        val audioParam = AudioParam(sampleRate, channelConfig, audioFormat, numChannels)
        // Camera1: SurfaceView  Camera2: TextureView
        pusher = Pusher(this, videoParam, audioParam, bind.surfaceView, CameraType.CAMERA2)
        if (bind.surfaceView is SurfaceView) {
            val holder: SurfaceHolder = (bind.surfaceView as SurfaceView).holder
            pusher?.setPreviewDisplay(holder)
        }


    }

    override fun onCheckedChanged(buttonView: CompoundButton, isChecked: Boolean) {
//        when (buttonView.id) {
//            R.id.btn_live//start or stop living
//            -> if (isChecked) {
//                pusher?.startPush(LIVE_URL, this)
//                isPushing = true
//            } else {
//                pusher?.stopPush()
//                isPushing = false
//            }
//
//            R.id.btn_mute//mute or not
//            -> {
//                Log.i(TAG, "isChecked=$isChecked")
//                pusher?.setMute(isChecked)
//            }
//
//            else -> {
//            }
//        }

        when (buttonView.id) {
            R.id.btn_live//start or stop living
            -> if (isChecked) {
                pusher?.startPush(LIVE_URL, this)
                isPushing = true
            } else {
                pusher?.stopPush()
                isPushing = false
            }

            R.id.btn_mute//mute or not
            -> {
                pusher?.setMute(isChecked)
            }

            else -> {
            }
        }
    }

    override fun onError(msg: String?) {
        mHandler.obtainMessage(MSG_ERROR, msg).sendToTarget()
    }

    override fun onNetworkChange() {
        if (pusher != null && isPushing) {
            pusher?.stopPush()
            isPushing = false
        }
    }

    override fun onDestroy() {
        super.onDestroy()
        orientationHandler?.disable()
        if (pusher != null) {
            if (isPushing) {
                isPushing = false
                pusher?.stopPush()
            }
            pusher?.release()
        }
        if (connectionReceiver != null) {
            unregisterReceiver(connectionReceiver)
        }
    }

    override fun onClick(view: View) {
        if (view.id == R.id.btn_swap) {//switch camera
            pusher?.switchCamera()
        }
    }
}