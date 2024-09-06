package com.mind.andffme

import android.Manifest
import android.os.Build
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.util.Log
import com.mind.andffme.databinding.ActivityMainBinding
import com.mind.andffme.day.testDay7
import com.permissionx.guolindev.PermissionX
import java.io.File
import java.io.FileOutputStream

class MainActivity : AppCompatActivity() {


    companion object {
        private const val TAG = "MainActivity"
        private const val FEATURE_FILE_NAME = "test.mp4"
    }


    private lateinit var binding: ActivityMainBinding

    private val permissionList = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
        listOf(
            Manifest.permission.WRITE_EXTERNAL_STORAGE,
            Manifest.permission.READ_EXTERNAL_STORAGE,
        )
    } else {
        listOf(
            Manifest.permission.WRITE_EXTERNAL_STORAGE,
            Manifest.permission.READ_EXTERNAL_STORAGE,
        )
    }


    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        PermissionX.init(this)
            .permissions(permissionList)
            .request { all, _, _ ->
                if (all) {
                    initView()
                    initFile()
                }
            }

    }

    private fun initView() {
        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)
        binding.sampleText.text = AndFFmeHelper.instance.stringFromJNI()

        testDay7(getFeatureFilePath())

    }


    private fun getFeatureFilePath() =
        getExternalFilesDir("ffmpeg")?.absolutePath + "/" + FEATURE_FILE_NAME

    private fun initFile() {
        val dir = File(getExternalFilesDir("ffmpeg")?.absolutePath)
        if (!dir.exists()) {
            dir.mkdirs()
        }
        val featureFile = File(getFeatureFilePath())
        if (!featureFile.exists()) {
            val copyModelFile = copyModelFile(FEATURE_FILE_NAME, R.raw.test)
            Log.d(TAG, "文件复制:$copyModelFile")
        } else {
            Log.d(TAG, "文件存在")
        }
    }


    private fun copyModelFile(fileName: String, id: Int): Boolean {
        val filePath = getExternalFilesDir("ffmpeg")?.absolutePath + "/" + fileName
        val file = File(filePath)
        try {
            if (!file.exists()) {
                val ins = resources.openRawResource(id) // 通过raw得到数据资源
                val fos = FileOutputStream(file)
                val buffer = ByteArray(8192)
                var count = 0
                while (ins.read(buffer).also { count = it } > 0) {
                    fos.write(buffer, 0, count)
                }
                fos.close()
                ins.close()
            }

            return true

        } catch (e: Exception) {
            Log.e(TAG, "fun createModelFile() is error:${e.message}")
        }

        return false
    }


}