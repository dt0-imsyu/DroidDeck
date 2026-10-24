package com.droiddeck.launcher.mavk

import android.app.Activity
import android.os.Bundle
import android.util.Log
import android.view.SurfaceHolder
import android.view.SurfaceView
import android.view.WindowManager
import com.droiddeck.launcher.gpu.SystemVulkanDriver
import com.droiddeck.launcher.wayland.CompositorHost
import com.droiddeck.launcher.wayland.WaylandCompositor
import java.io.File

class MavkWindowActivity : Activity(), SurfaceHolder.Callback {
    private lateinit var view: SurfaceView
    private lateinit var runDir: File
    private var ready = false
    private var attached = false
    private val frameListener = Runnable {
        Log.i("MavkWindow", "ACTUAL_COMPOSITOR_FIRST_FRAME")
        File(runDir, "first-frame.txt").writeText(System.nanoTime().toString())
    }

    override fun onCreate(state: Bundle?) {
        super.onCreate(state)
        window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
        runDir = File(filesDir, "mavk-window/run").apply { mkdirs() }
        File(runDir, "native-library-dir.txt").writeText(applicationInfo.nativeLibraryDir)
        view = SurfaceView(this)
        view.holder.addCallback(this)
        setContentView(view)
        WaylandCompositor.setFirstFrameListener(frameListener)
        Thread({
            try {
                val runtime = SystemVulkanDriver.prepare(this)
                File(runDir, "runtime.txt").writeText(runtime.absolutePath)
                runOnUiThread { ready = true; attach() }
            } catch (error: Exception) {
                Log.e("MavkWindow", "RUNTIME_PREPARE_FAILED", error)
                File(runDir, "error.txt").writeText(error.toString())
            }
        }, "MavkPrepare").start()
    }

    private fun attach() {
        if (!ready || attached || !view.holder.surface.isValid) return
        CompositorHost.startOrAttach(view.holder.surface, runDir.absolutePath,
            null, null, applicationInfo.nativeLibraryDir, 1280, 800, 60f, 60)
        attached = true
        Log.i("MavkWindow", "COMPOSITOR_STARTED runtime=${runDir.absolutePath}")
    }

    override fun surfaceCreated(holder: SurfaceHolder) = attach()
    override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {
        if (attached) CompositorHost.resize(holder.surface) {} else attach()
    }
    override fun surfaceDestroyed(holder: SurfaceHolder) {
        if (attached) CompositorHost.detach(holder.surface)
        attached = false
    }
    override fun onDestroy() {
        WaylandCompositor.clearFirstFrameListener(frameListener)
        if (attached) CompositorHost.detach(view.holder.surface)
        super.onDestroy()
    }
}
