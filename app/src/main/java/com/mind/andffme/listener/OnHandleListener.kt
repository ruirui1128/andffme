package com.mind.andffme.listener


interface OnHandleListener {
    fun onBegin()
    fun onMsg(msg: String)
    fun onProgress(progress: Int, duration: Int)
    fun onEnd(resultCode: Int, resultMsg: String)
}
