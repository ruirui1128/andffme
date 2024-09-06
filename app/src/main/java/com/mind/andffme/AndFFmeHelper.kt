package com.mind.andffme

class AndFFmeHelper {
    companion object{

        val instance  by lazy(LazyThreadSafetyMode.SYNCHRONIZED) { AndFFmeHelper() }

        init {
            // native-lib
            System.loadLibrary("andffme")
        }
    }

    external fun stringFromJNI(): String
}