package com.droiddeck.launcher.wayland

import org.junit.Assert.assertEquals
import org.junit.Test

class FrameLoopTest {
    private class Harness {
        var queued = 0
        val rendered = mutableListOf<Long>()
        val loop = FrameLoop({ queued++ }, { queued = 0 }, { rendered.add(it) })

        fun tick(time: Long) {
            assertEquals(1, queued)
            queued--
            loop.onFrame(time)
        }
    }

    @Test fun repeatedStartKeepsOneCallback() {
        val h = Harness()
        repeat(10) { h.loop.start() }
        h.tick(123)
        assertEquals(listOf(123L), h.rendered)
        assertEquals(1, h.queued)
    }

    @Test fun rapidDetachAttachDoesNotMultiplyCallbacks() {
        val h = Harness()
        h.loop.start()
        repeat(100) { h.loop.stop(); h.loop.start() }
        repeat(60) { h.tick(it.toLong()) }
        assertEquals(60, h.rendered.size)
        assertEquals(1, h.queued)
    }

    @Test fun stoppedCallbackDoesNotRenderOrRepost() {
        val h = Harness()
        h.loop.start()
        h.loop.stop()
        h.loop.onFrame(321)
        assertEquals(0, h.queued)
        assertEquals(0, h.rendered.size)
    }

    @Test fun stopDuringRenderDoesNotRepost() {
        var queued = 0
        lateinit var loop: FrameLoop
        loop = FrameLoop({ queued++ }, { queued = 0 }, { loop.stop() })
        loop.start()
        queued--
        loop.onFrame(456)
        assertEquals(0, queued)
    }
}
