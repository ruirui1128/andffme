package com.mind.andffme.rtmp

import android.app.Dialog
import android.content.Context
import android.content.Intent
import android.graphics.BitmapFactory
import android.os.Bundle
import android.view.GestureDetector
import android.view.MotionEvent
import android.view.View
import android.widget.EditText
import android.widget.ImageButton
import android.widget.ProgressBar
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import com.dwayne.com.camera.CameraListener
import com.dwayne.com.configuration.AudioConfiguration
import com.dwayne.com.configuration.CameraConfiguration
import com.dwayne.com.configuration.VideoConfiguration
import com.dwayne.com.entity.Watermark
import com.dwayne.com.entity.WatermarkPosition
import com.dwayne.com.stream.sender.nativertmp.RtmpNativeSendr
import com.dwayne.com.ui.SoftLivingView
import com.dwayne.com.utils.SopCastLog
import com.dwayne.com.video.effect.GrayEffect
import com.dwayne.com.video.effect.NullEffect
import com.mind.andffme.R
import com.mind.andffme.view.MultiToggleImageButton
import kotlin.math.abs


fun launchRtmpPush(context: Context) {
    context.startActivity(Intent(context, RtmpPushActivity::class.java))
}

class RtmpPushActivity : AppCompatActivity() {

    private lateinit var mProgressConnecting: ProgressBar
    private lateinit var mMicBtn: MultiToggleImageButton
    private lateinit var mFlashBtn: MultiToggleImageButton
    private lateinit var mFaceBtn: MultiToggleImageButton
    private lateinit var mBeautyBtn: MultiToggleImageButton
    private lateinit var mFocusBtn: MultiToggleImageButton
    private lateinit var mRecordBtn: ImageButton
    private lateinit var mGestureDetector: GestureDetector
    private lateinit var mLFLiveView: SoftLivingView

    private lateinit var mRtmpSender: RtmpNativeSendr
    private lateinit var mVideoConfiguration: VideoConfiguration


    private lateinit var mGrayEffect: GrayEffect
    private lateinit var mNullEffect: NullEffect
    private var isGray = false
    private var isRecording = false


    private var mUploadDialog: Dialog? = null
    private var mAddressET: EditText? = null

    private companion object {
        private const val PUSH_URL = "rtmp://192.168.20.53/live/mystream110"
    }


    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_live)
        initEffects()
        initViews()
        initListeners()
        initLiveView()
    }

    private fun initEffects() {
        mGrayEffect = GrayEffect(this)
        mNullEffect = NullEffect(this)
    }

    private fun initViews() {
        mLFLiveView = findViewById<View>(R.id.liveView) as SoftLivingView
        mMicBtn = findViewById<View>(R.id.record_mic_button) as MultiToggleImageButton
        mFlashBtn = findViewById<View>(R.id.camera_flash_button) as MultiToggleImageButton
        mFaceBtn = findViewById<View>(R.id.camera_switch_button) as MultiToggleImageButton
        mBeautyBtn = findViewById<View>(R.id.camera_render_button) as MultiToggleImageButton
        mFocusBtn = findViewById<View>(R.id.camera_focus_button) as MultiToggleImageButton
        mRecordBtn = findViewById<View>(R.id.btnRecord) as ImageButton
        mProgressConnecting = findViewById<View>(R.id.progressConnecting) as ProgressBar
        mLFLiveView.visibility = View.VISIBLE


    }


    private fun initListeners() {
        mMicBtn.setOnStateChangeListener { view, state ->
            //                mLFLiveView.mute(true);
        }
        mFlashBtn.setOnStateChangeListener { view, state -> mLFLiveView.switchTorch() }
        mFaceBtn.setOnStateChangeListener { view, state -> mLFLiveView.switchCamera() }
        mBeautyBtn.setOnStateChangeListener { view, state ->
            if (isGray) {
                mLFLiveView.setEffect(mNullEffect)
                isGray = false
            } else {
                mLFLiveView.setEffect(mGrayEffect)
                isGray = true
            }
        }
        mFocusBtn.setOnStateChangeListener { view, state -> mLFLiveView.switchFocusMode() }
        mRecordBtn.setOnClickListener {
            if (isRecording) {
                mProgressConnecting.visibility = View.GONE
                mRecordBtn.setBackgroundResource(R.mipmap.ic_record_start)
                mLFLiveView.stop()
                isRecording = false
            } else {
                isRecording = true
                mRtmpSender.setAddress(PUSH_URL)
                //开始连接
                mRtmpSender.connect()
            }
        }

        //设置预览监听
        mLFLiveView.setCameraOpenListener(object : CameraListener {
            override fun onOpenSuccess() {
                Toast.makeText(applicationContext, "camera open success", Toast.LENGTH_LONG).show()
            }

            override fun onOpenFail(error: Int) {
                Toast.makeText(applicationContext, "camera open fail", Toast.LENGTH_LONG).show()
            }

            override fun onCameraChange() {
                Toast.makeText(applicationContext, "camera switch", Toast.LENGTH_LONG).show()
            }
        })


        mLFLiveView.setLivingStartListener(object : SoftLivingView.LivingStartListener {
            override fun startError(error: Int) {
                //直播失败
                Toast.makeText(applicationContext, "start living fail", Toast.LENGTH_SHORT).show()
                mLFLiveView.stop()
            }

            override fun startSuccess() {
                //直播成功
                Toast.makeText(applicationContext, "start living", Toast.LENGTH_SHORT).show()
            }
        })
    }

    private fun initLiveView() {
        SopCastLog.isOpen(true)
        mLFLiveView.init()
        val cameraBuilder = CameraConfiguration.Builder()
        cameraBuilder.setOrientation(CameraConfiguration.Orientation.LANDSCAPE)
            .setFacing(CameraConfiguration.Facing.BACK)
        val cameraConfiguration = cameraBuilder.build()
        mLFLiveView.setCameraConfiguration(cameraConfiguration)
        //语音参数配置
        setAudioInfo()
        //视频参数配置
        setVideoInfo()
        //设置水印
        setWatermark()
        //设置手势识别
        setGestureDetector()
        //设置发送器
        setNativeRtmpPush()
    }


    /**
     * 设置视频编码信息
     */
    private fun setVideoInfo() {
        mVideoConfiguration = VideoConfiguration.Builder()
            .setSize(1280, 720)
            .setBps(300, 500)
            .setFps(20)
            .setMediaCodec(false)
            .build()
        mLFLiveView.setVideoConfiguration(mVideoConfiguration)
    }

    /**
     * 设置语音编码信息
     */
    private fun setAudioInfo() {
        val mAudioConfig = AudioConfiguration.Builder().setMediaCodec(false).build()

        mLFLiveView.setAudioConfiguration(mAudioConfig)
    }


    private fun setWatermark() {
        val watermarkImg = BitmapFactory.decodeResource(
            resources, R.mipmap.ic_launcher
        )
        val watermark = Watermark(
            watermarkImg,
            50,
            25,
            WatermarkPosition.WATERMARK_ORIENTATION_BOTTOM_RIGHT,
            8,
            8
        )
        mLFLiveView.setWatermark(watermark)
    }

    private fun setGestureDetector() {
        mGestureDetector = GestureDetector(this.applicationContext, GestureListener())
        mLFLiveView.setOnTouchListener { v, event ->
            mGestureDetector.onTouchEvent(event)
            false
        }
    }

    class GestureListener : GestureDetector.SimpleOnGestureListener() {
        override fun onFling(
            e1: MotionEvent?,
            e2: MotionEvent,
            velocityX: Float,
            velocityY: Float
        ): Boolean {
            if (e1!!.x - e2.x > 100
                && abs(velocityX.toDouble()) > 200
            ) {
                // Fling left
                //  Toast.makeText(, "Fling Left", Toast.LENGTH_SHORT).show()
            } else if (e2.x - e1.x > 100
                && abs(velocityX.toDouble()) > 200
            ) {
                // Fling right
                // Toast.makeText(this, "Fling Right", Toast.LENGTH_SHORT).show()
            }
            return super.onFling(e1, e2, velocityX, velocityY)
        }
    }

    private val mSenderListener: RtmpNativeSendr.OnSenderListener =
        object : RtmpNativeSendr.OnSenderListener {
            /**
             * 开始链接
             */
            override fun onConnecting() {
                mProgressConnecting.visibility = View.VISIBLE
                Toast.makeText(applicationContext, "start connecting", Toast.LENGTH_SHORT).show()
                mRecordBtn.setBackgroundResource(R.mipmap.ic_record_stop)
                isRecording = true
            }

            /**
             * 链接成功
             */
            override fun onConnected() {
                mProgressConnecting.visibility = View.GONE
                mLFLiveView.start()
            }

            /**
             * 取消链接
             */
            override fun onDisConnected() {
                mProgressConnecting.visibility = View.GONE
                Toast.makeText(applicationContext, "fail to live", Toast.LENGTH_SHORT).show()
                mRecordBtn.setBackgroundResource(R.mipmap.ic_record_start)
                mLFLiveView.stop()
                isRecording = false
            }

            /**
             * 推送失败
             */
            override fun onPublishFail() {
                mProgressConnecting.visibility = View.GONE
                Toast.makeText(applicationContext, "fail to publish stream", Toast.LENGTH_SHORT)
                    .show()
                mRecordBtn.setBackgroundResource(R.mipmap.ic_record_start)
                isRecording = false
            }

            /**
             * rtmp 其它处理失败
             * @param error
             */
            override fun otherFail(error: String) {
                mProgressConnecting.visibility = View.GONE
                Toast.makeText(
                    applicationContext,
                    "fail to publish stream：$error", Toast.LENGTH_SHORT
                ).show()
                mRecordBtn.setBackgroundResource(R.mipmap.ic_record_start)
                isRecording = false
            }
        }

    /**
     * 此为扩展函数
     */
    private fun setNativeRtmpPush() {
        mRtmpSender = RtmpNativeSendr()
        mRtmpSender.setSenderListener(mSenderListener)
        mLFLiveView.setSender(mRtmpSender)
    }


    override fun onStop() {
        super.onStop()
        mLFLiveView.pause()
    }

    override fun onStart() {
        super.onStart()
        mLFLiveView.resume()
    }

    override fun onDestroy() {
        super.onDestroy()
        mLFLiveView.stop()
        mLFLiveView.release()
        mGestureDetector
    }


}