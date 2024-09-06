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

}