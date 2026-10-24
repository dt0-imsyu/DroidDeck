package com.droiddeck.launcher.session

import android.content.Context
import com.droiddeck.launcher.core.FileUtils
import java.io.File
import java.security.MessageDigest
import java.util.UUID
import java.util.zip.ZipInputStream
import org.json.JSONObject

internal object MavkRuntime {
    @Synchronized
    fun prepare(context: Context): File {
        val version = context.assets.open("mavk/version.txt").bufferedReader().use { it.readText().trim() }
        check(version.matches(Regex("[0-9a-f]{64}"))) { "Invalid Mavk runtime version" }
        val base = File(context.filesDir, "mavk-runtime").apply { mkdirs() }
        val target = File(base, version)
        if (File(target, "installed.ready").readTextOrNull() == version) return target
        check(!target.exists()) { "Incomplete Mavk runtime; no files were replaced" }
        val stage = File(base, ".stage-${UUID.randomUUID()}").apply { check(mkdir()) }
        try {
            val digest = MessageDigest.getInstance("SHA-256")
            context.assets.open("mavk/runtime.zip").use { input ->
                val buffer = ByteArray(65536)
                while (true) {
                    val count = input.read(buffer)
                    if (count < 0) break
                    digest.update(buffer, 0, count)
                }
            }
            check(hex(digest.digest()) == version) { "Mavk runtime archive checksum mismatch" }
            val names = mutableSetOf<String>()
            var total = 0L
            ZipInputStream(context.assets.open("mavk/runtime.zip")).use { zip ->
                val buffer = ByteArray(65536)
                while (true) {
                    val entry = zip.nextEntry ?: break
                    val name = entry.name
                    check(!entry.isDirectory && name.isNotEmpty() && !name.startsWith('/') &&
                        '\\' !in name && name.split('/').none { it.isEmpty() || it == "." || it == ".." } &&
                        names.add(name) && names.size <= 512) { "Unsafe Mavk runtime entry" }
                    val file = File(stage, name)
                    check(file.parentFile!!.isDirectory || file.parentFile!!.mkdirs())
                    var bytes = 0L
                    file.outputStream().use { output ->
                        while (true) {
                            val count = zip.read(buffer)
                            if (count < 0) break
                            bytes += count
                            total += count
                            check(bytes <= 32L * 1024 * 1024 && total <= 128L * 1024 * 1024) {
                                "Mavk runtime exceeds extraction limit"
                            }
                            output.write(buffer, 0, count)
                        }
                    }
                    zip.closeEntry()
                }
            }
            val manifestFile = File(stage, "manifest.json")
            check(manifestFile.length() <= 256 * 1024) { "Mavk manifest exceeds limit" }
            val manifest = JSONObject(manifestFile.readText())
            check(names == manifest.keys().asSequence().toSet() + "manifest.json") { "Mavk manifest entry mismatch" }
            for (name in manifest.keys()) {
                val item = manifest.getJSONObject(name)
                val file = File(stage, name)
                check(file.length() == item.getLong("bytes")) { "Mavk runtime size mismatch: $name" }
                digest.reset()
                file.inputStream().use { input ->
                    val buffer = ByteArray(65536)
                    while (true) {
                        val count = input.read(buffer)
                        if (count < 0) break
                        digest.update(buffer, 0, count)
                    }
                }
                check(hex(digest.digest()) == item.getString("sha256")) { "Mavk runtime checksum mismatch: $name" }
                if (item.getBoolean("executable")) check(file.setExecutable(true, true))
            }
            File(stage, "installed.ready").writeText(version)
            check(stage.renameTo(target)) { "Cannot publish Mavk runtime" }
            return target
        } finally {
            if (stage.exists()) FileUtils.delete(stage)
        }
    }

    private fun File.readTextOrNull(): String? = if (isFile) readText() else null
    private fun hex(bytes: ByteArray): String = bytes.joinToString("") { "%02x".format(it.toInt() and 255) }
}
