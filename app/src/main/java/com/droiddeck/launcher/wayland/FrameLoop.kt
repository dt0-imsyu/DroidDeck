package com.droiddeck.launcher.wayland

internal class FrameLoop(
    private val post: () -> Unit,
    private val cancel: () -> Unit,
    private val render: (Long) -> Unit,
) {
    private var running = false

    fun start() {
        if (running) return
        running = true
        post()
    }

    fun stop() {
        running = false
        cancel()
    }

    fun onFrame(timeNanos: Long) {
        if (!running) return
        render(timeNanos)
        if (running) post()
    }
}
