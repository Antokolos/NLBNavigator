package com.nlbhub.nlbnavigator.launcher

import android.content.Context
import android.net.Uri
import org.json.JSONObject
import java.io.BufferedInputStream
import java.io.File
import java.io.IOException
import java.io.InputStream
import java.net.HttpURLConnection
import java.net.URL
import java.util.zip.ZipInputStream

/**
 * Установленные игры: <files>/games/<id>/book.nlbpack и meta.json.
 *
 * Устанавливается .nlbz (ZIP: book.nlbpack + meta.json) или просто .nlbpack (тогда описание —
 * по имени файла). Источник — файл с устройства (системный выбор файла) или ссылка из каталога.
 */
class Library(private val context: Context) {
    private val root = File(context.filesDir, "games").apply { mkdirs() }

    data class Installed(val info: GameInfo, val book: File)

    fun list(): List<Installed> = (root.listFiles() ?: emptyArray())
        .filter { File(it, BOOK).isFile }
        .mapNotNull { dir ->
            val info = readMeta(dir) ?: return@mapNotNull null
            Installed(info, File(dir, BOOK))
        }
        .sortedBy { it.info.title().lowercase() }

    fun find(id: String): Installed? = list().firstOrNull { it.info.id == id }

    fun delete(id: String) {
        File(root, safeId(id)).deleteRecursively()
    }

    /** Установить из файла устройства (Uri от системного выбора файла) */
    fun installFromUri(uri: Uri, displayName: String, progress: (Long) -> Unit): GameInfo {
        val input = context.contentResolver.openInputStream(uri) ?: throw IOException("не открыть файл")
        return input.use { install(it, displayName, null, progress) }
    }

    /** Скачать и установить игру из каталога */
    fun installFromCatalog(game: GameInfo, progress: (Long) -> Unit): GameInfo {
        val conn = URL(game.url).openConnection() as HttpURLConnection
        conn.connectTimeout = 15000
        conn.readTimeout = 30000
        conn.instanceFollowRedirects = true
        try {
            if (conn.responseCode !in 200..299) throw IOException("HTTP ${conn.responseCode}")
            return conn.inputStream.use { install(it, game.url.substringAfterLast('/'), game, progress) }
        } finally {
            conn.disconnect()
        }
    }

    /**
     * Поток — .nlbz (начинается с «PK») или .nlbpack. Распаковывается во временный каталог и
     * только потом заменяет установленную игру (прерванная установка ничего не портит).
     */
    private fun install(raw: InputStream, fileName: String, catalog: GameInfo?, progress: (Long) -> Unit): GameInfo {
        val tmp = File(root, ".installing").apply { deleteRecursively(); mkdirs() }
        try {
            val input = BufferedInputStream(raw)
            input.mark(4)
            val isZip = input.read() == 'P'.code && input.read() == 'K'.code
            input.reset()
            var meta: GameInfo? = null
            var counted = 0L
            val counting = { n: Int -> counted += n; progress(counted) }
            if (isZip) {
                ZipInputStream(input).use { zip ->
                    while (true) {
                        val entry = zip.nextEntry ?: break
                        // только эти два файла, по их именам — никаких путей из архива
                        when (entry.name) {
                            BOOK -> copy(zip, File(tmp, BOOK), counting)
                            META -> meta = GameInfo.fromJson(JSONObject(zip.readBytes().toString(Charsets.UTF_8)))
                        }
                    }
                }
            } else {
                copy(input, File(tmp, BOOK), counting)
            }
            if (!File(tmp, BOOK).isFile) throw IOException("в архиве нет book.nlbpack")
            val base = fileName.substringBeforeLast('.')
            val info = (meta ?: catalog ?: GameInfo(base, "", mapOf("" to base), emptyMap(), emptyMap(), emptyMap(), "vn", ""))
                .let { m -> if (catalog != null) m.copy(id = catalog.id, thumbnail = catalog.thumbnail, screenshot = catalog.screenshot) else m }
                .let { m -> if (m.id.isBlank()) m.copy(id = base) else m }
            File(tmp, META).writeText(info.toJson().toString(2))
            val target = File(root, safeId(info.id))
            target.deleteRecursively()
            if (!tmp.renameTo(target)) throw IOException("не удалось установить игру")
            return info
        } finally {
            tmp.deleteRecursively()
        }
    }

    private fun copy(input: InputStream, file: File, counted: (Int) -> Unit) {
        file.outputStream().use { out ->
            val buf = ByteArray(1 shl 16)
            while (true) {
                val n = input.read(buf)
                if (n < 0) break
                out.write(buf, 0, n)
                counted(n)
            }
        }
    }

    private fun readMeta(dir: File): GameInfo? = try {
        GameInfo.fromJson(JSONObject(File(dir, META).readText()))
    } catch (e: Exception) {
        null
    }

    private fun safeId(id: String) = id.replace(Regex("[^A-Za-z0-9._-]"), "_").ifEmpty { "game" }

    companion object {
        const val BOOK = "book.nlbpack"
        const val META = "meta.json"
    }
}
