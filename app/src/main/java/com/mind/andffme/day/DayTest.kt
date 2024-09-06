package com.mind.andffme.day

import android.content.Context
import com.mind.andffme.IoScope
import kotlinx.coroutines.delay
import kotlinx.coroutines.launch


fun testDay7(context: Context, path: String) {
    // DayPresenter.instance.stringFromJNI()

    IoScope().launch {
        DayPresenter.instance.printVideoInfo(path = path)
        delay(100L)
        val h264Path = context.getExternalFilesDir("ffmpeg")?.absolutePath + "/" + "test.h264"
        val aacPath = context.getExternalFilesDir("ffmpeg")?.absolutePath + "/" + "test.aac"
        DayPresenter.instance.demuxMp4(path, h264Path, aacPath)
    }
}


