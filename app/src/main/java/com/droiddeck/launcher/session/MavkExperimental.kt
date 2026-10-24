package com.droiddeck.launcher.session

import android.content.Context
import android.os.Environment
import android.os.Process
import android.os.SystemClock
import android.util.Log
import com.droiddeck.launcher.core.HostProcess
import com.droiddeck.launcher.core.SessionPart
import com.droiddeck.launcher.gpu.SystemVulkanDriver
import com.droiddeck.launcher.runtime.LinuxRuntime
import java.io.File
import java.io.DataOutputStream
import java.security.SecureRandom
import java.util.concurrent.TimeUnit

internal object MavkExperimental {
    private var ahbPrepared: Boolean? = null

    fun ahbDisplay(context: Context): Boolean = context.getSharedPreferences("mavk", Context.MODE_PRIVATE)
        .getBoolean("ahb_display", false)

    fun selectAhbDisplay(context: Context, enabled: Boolean) {
        context.getSharedPreferences("mavk", Context.MODE_PRIVATE).edit().putBoolean("ahb_display", enabled).apply()
    }

    private fun presentation(directory: File): Pair<File, String> {
        val component = File(directory, "presentation/ahb")
        val receiptFile = File(component, "mavk-ahb.json")
        check(receiptFile.isFile && receiptFile.length() <= 4096) { "AHB component receipt missing" }
        val receipt = org.json.JSONObject(receiptFile.readText())
        check(receipt.getInt("version") == 1 && receipt.getInt("min_images") == 3 &&
            receipt.getBoolean("bgra8_gpu_blit") && receipt.getJSONArray("present_modes").toString() == "[\"fifo\"]")
        val expected = receipt.getString("library_sha256")
        check(expected.matches(Regex("[0-9a-f]{64}")))
        val library = File(component, "libVkLayer_window_system_integration.so")
        check(library.isFile && library.length() in 64L..32L * 1024 * 1024)
        val actual = java.security.MessageDigest.getInstance("SHA-256").digest(library.readBytes())
            .joinToString("") { "%02x".format(it.toInt() and 255) }
        check(actual == expected && File(component, "VkLayer_window_system_integration.json").isFile)
        check(File(directory, "integration/droiddeck-game-env").canExecute())
        return component to expected
    }

    fun prepareDisplay(context: Context, runtimeDir: File, width: Int, height: Int, mode: String) {
        val wanted = available(context) && selected(context) && ahbDisplay(context) &&
            mode in setOf(SessionService.MODE_STEAM, SessionService.MODE_RUN) &&
            SystemVulkanDriver.usesDefault(SessionPrefs.linuxDriver(context)) && !SessionState.hdr
        if (com.droiddeck.launcher.wayland.CompositorHost.isStarted) {
            if (wanted && ahbPrepared != true) {
                android.widget.Toast.makeText(context, "Restart DroidDeck to enable AHB; this session uses X11/SHM", android.widget.Toast.LENGTH_LONG).show()
            }
            return
        }
        ahbPrepared = false
        val marker = File(runtimeDir, "mavk-explicit-ahb.enable")
        runCatching {
            check(!marker.exists() || marker.delete()) { "Cannot clear stale AHB marker" }
            if (wanted) {
                check(width in 320..4096 && height in 240..4096 && width % 2 == 0 && height % 2 == 0)
                presentation(MavkRuntime.prepare(context))
                marker.writeText("$width $height\n")
                ahbPrepared = true
            }
        }.onFailure {
            marker.delete()
            Log.w("MavkSession", "AHB preparation failed; using X11/SHM", it)
            if (wanted) android.widget.Toast.makeText(context, "AHB unavailable; using X11/SHM", android.widget.Toast.LENGTH_LONG).show()
        }
    }

    fun available(context: Context): Boolean = context.packageName.endsWith(".mavk") && SystemVulkanDriver.isMali()

    fun selected(context: Context): Boolean = context.getSharedPreferences("mavk", Context.MODE_PRIVATE)
        .getBoolean("enabled", false)

    fun select(context: Context, enabled: Boolean) {
        context.getSharedPreferences("mavk", Context.MODE_PRIVATE).edit().putBoolean("enabled", enabled).apply()
    }

    fun steamHardwareUi(context: Context): Boolean = context.getSharedPreferences("mavk", Context.MODE_PRIVATE)
        .getBoolean("steam_hardware_ui", true)

    fun selectSteamHardwareUi(context: Context, enabled: Boolean) {
        context.getSharedPreferences("mavk", Context.MODE_PRIVATE).edit().putBoolean("steam_hardware_ui", enabled).apply()
    }

    fun enabled(context: Context): Boolean = available(context) && selected(context) &&
        SessionState.mode in setOf(SessionService.MODE_STEAM, SessionService.MODE_RUN) &&
        SystemVulkanDriver.usesDefault(SessionPrefs.linuxDriver(context))

    fun configure(context: Context, environment: MutableList<String>) {
        val directory = MavkRuntime.prepare(context)
        val work = File(context.filesDir, "mavk-session").apply { mkdirs() }
        check(File(directory, "payload/libVkLayer_mavk_compat.so").isFile) { "Mavk layer is not staged" }
        val runtime = SystemVulkanDriver.prepare(context)
        val policy = File(directory, "integration/droiddeck-game-env")
        if (policy.isFile && policy.canExecute()) {
            environment += "DROIDDECK_GAME_ENV_LAUNCHER=${policy.path}"
            val useAhb = ahbDisplay(context) && ahbPrepared == true
            environment += "DROIDDECK_MAVK_PRESENTATION=" + if (useAhb) "ahb-bgra" else "x11-shm"
            if (useAhb) {
                val (component, hash) = presentation(directory)
                environment += "DROIDDECK_MAVK_XCB_AHB_WSI=${component.path}"
                environment += "DROIDDECK_MAVK_XCB_AHB_SHA256=$hash"
            }
        }
        environment.removeAll { it.startsWith("DXVK_MALI_COMPAT=") || it.startsWith("LD_LIBRARY_PATH=") }
        environment += listOf(
            "DROIDDECK_MAVK_DIRECT=1", "DISPLAY=:99", "BL_SYSV_DIR=${File(work, "sysv").path}",
            "DROIDDECK_MAVK_RUNTIME=${directory.path}",
            "DROIDDECK_MAVK_GUEST_ROOT=${LinuxRuntime.rootDir(context).path}",
            "DROIDDECK_SYSTEM_VULKAN_DXVK=managed",
            "XAUTHORITY=${File(work, "xauthority").path}",
            "DROIDDECK_MAVK_LAYER_PATH=${File(directory, "payload").path}",
            "DROIDDECK_HYBRIS_SESSION_LAYERS=VK_LAYER_MAVK_compat:VK_LAYER_HYBRIS_compat",
            "DROIDDECK_HYBRIS_LAYERS=VK_LAYER_MAVK_compat:VK_LAYER_HYBRIS_compat",
            "LD_LIBRARY_PATH=/usr/lib:${File(runtime, "lib").path}",
            "MAVK_LAYER_GUARDS=0", "DROIDDECK_VR_LAYER_DISABLE=1",
            "BL_STEAM_CEF_SOFTWARE=1",
        )
        if (environment.none { it.startsWith("DROIDDECK_MAVK_STEAM_GL=") }) {
            environment += "DROIDDECK_MAVK_STEAM_GL=" + if (steamHardwareUi(context)) "1" else "0"
        }
    }
}

internal class MavkXwaylandComponent(
    private val context: Context,
    private val runtimeDir: File,
    private val sessionRoot: File,
    private val sessionDir: File,
    private val width: Int,
    private val height: Int,
    private val terminateTree: (Int) -> Unit,
) : SessionPart() {
    private var process: java.lang.Process? = null
    private var pid = -1

    override fun start() {
        val directory = MavkRuntime.prepare(context)
        val work = File(context.filesDir, "mavk-session").apply { mkdirs() }
        val helper = File(directory, "pe/libmavk-x11-sysv-owner.so")
        check(helper.isFile) { "Mavk X11 shared-memory helper is not staged" }
        val sysv = File(work, "sysv").apply { mkdirs() }
        val authority = File(work, "xauthority")
        val cookie = ByteArray(16).also { SecureRandom().nextBytes(it) }
        DataOutputStream(authority.outputStream()).use { out ->
            out.writeShort(65535)
            for (field in listOf(byteArrayOf(), "99".toByteArray(), "MIT-MAGIC-COOKIE-1".toByteArray(), cookie)) {
                out.writeShort(field.size)
                out.write(field)
            }
        }
        authority.setReadable(false, false)
        check(authority.setReadable(true, true)) { "Cannot protect Mavk X authority file" }
        val root = LinuxRuntime.rootDir(context)
        val socket = File(root, "tmp/.X11-unix/X99")
        check(!socket.exists() || socket.delete()) { "Stale Mavk X11 socket cannot be removed" }
        val command = LinuxRuntime.prootPrefix(context, root, "/root", true)
        LinuxRuntime.binds(context, sessionRoot, runtimeDir, Environment.getExternalStorageDirectory(), null)
            .forEach { command += listOf("-b", it) }
        // The root facade is only for Xwayland's -nolock check; Android retains the app UID.
        command += listOf(
            "/usr/bin/env", "-i", "HOME=/root", "PATH=/usr/local/bin:/usr/bin:/bin",
            "XDG_RUNTIME_DIR=${LinuxRuntime.GUEST_RUNTIME_DIR}", "WAYLAND_DISPLAY=wayland-0",
            "BL_SYSV_DIR=${sysv.path}", "MAVK_X11_SYSV_OWNER=${Process.myUid()}",
            "LD_PRELOAD=${helper.path}", "/usr/bin/Xwayland", ":99", "-nolock",
            "-noreset", "-nolisten", "tcp", "-auth", authority.path, "-shm", "-geometry", "${width}x${height}",
        )
        val builder = ProcessBuilder(command).directory(root).redirectErrorStream(true)
            .redirectOutput(File(sessionDir, "mavk-xwayland.log"))
        builder.environment().apply {
            put("PROOT_LOADER", LinuxRuntime.prootLoader(context).path)
            put("PROOT_TMP_DIR", context.cacheDir.path)
            remove("LD_PRELOAD")
            LinuxRuntime.prootLibraryPath(context).takeIf { it.isNotEmpty() }?.let { put("LD_LIBRARY_PATH", it) }
        }
        val child = builder.start()
        process = child
        pid = HostProcess.pidOf(child)
        OrphanReaper.keep(pid)
        val deadline = SystemClock.elapsedRealtime() + 10_000L
        while (!socket.exists() && child.isAlive && SystemClock.elapsedRealtime() < deadline) Thread.sleep(50)
        if (!child.isAlive || !socket.exists()) {
            stop()
            error("Mavk Xwayland did not start; see mavk-xwayland.log")
        }
        Log.i("MavkSession", "Xwayland ready :99 ${width}x${height}, Android uid ${Process.myUid()}")
    }

    override fun stop() {
        val child = process ?: return
        process = null
        if (pid > 1) terminateTree(pid)
        child.destroy()
        if (!child.waitFor(1, TimeUnit.SECONDS)) child.destroyForcibly()
        OrphanReaper.release(pid)
        pid = -1
    }

    override fun suspendPid(): Int = pid
}
