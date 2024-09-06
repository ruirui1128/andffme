package com.mind.andffme.day

class DayPresenter {
    companion object {
        val instance by lazy(LazyThreadSafetyMode.SYNCHRONIZED) { DayPresenter() }

        init {
            // native-lib
            System.loadLibrary("andffme")
        }
    }

    external fun stringFromJNI(): String

    external fun printVideoInfo(path: String)

    external fun demuxMp4(path: String, h264Path: String, aacPath: String):Int

}