package com.nlbhub.nlbnavigator.launcher

import org.json.JSONObject
import java.util.Locale

/**
 * Описание игры — одно и то же в каталоге (catalog.json) и внутри .nlbz (meta.json):
 * названия, авторы и описания по языкам, версия, режим книги, а в каталоге ещё размер архива и
 * ссылки на архив, миниатюру и скриншот.
 */
data class GameInfo(
    val id: String,
    val version: String,
    val title: Map<String, String>,
    val author: Map<String, String>,
    val summary: Map<String, String>,
    val description: Map<String, String>,
    val mode: String,
    val args: String,
    val size: Long = 0,
    val url: String = "",
    val thumbnail: String = "",
    val screenshot: String = "",
) {
    fun title(): String = localized(title).ifEmpty { id }
    fun author(): String = localized(author)
    fun summary(): String = localized(summary)
    fun description(): String = localized(description).ifEmpty { summary() }

    /** Аргументы плеера: режим книги и ключи */
    fun playerArgs(): String = listOf(mode, args).filter { it.isNotBlank() }.joinToString(" ")

    fun toJson(): JSONObject = JSONObject().apply {
        put("id", id); put("version", version); put("mode", mode); put("args", args)
        put("title", JSONObject(title)); put("author", JSONObject(author))
        put("summary", JSONObject(summary)); put("description", JSONObject(description))
        if (size > 0) put("size", size)
        if (url.isNotEmpty()) put("url", url)
        if (thumbnail.isNotEmpty()) put("thumbnail", thumbnail)
        if (screenshot.isNotEmpty()) put("screenshot", screenshot)
    }

    companion object {
        fun fromJson(o: JSONObject): GameInfo = GameInfo(
            id = o.optString("id"),
            version = o.optString("version"),
            title = langMap(o.opt("title")),
            author = langMap(o.opt("author")),
            summary = langMap(o.opt("summary")),
            description = langMap(o.opt("description")),
            mode = o.optString("mode", "vn"),
            args = o.optString("args"),
            size = o.optLong("size"),
            url = o.optString("url"),
            thumbnail = o.optString("thumbnail"),
            screenshot = o.optString("screenshot"),
        )

        /** {"ru": "…", "en": "…"} или просто строка (тогда — для всех языков) */
        private fun langMap(v: Any?): Map<String, String> = when (v) {
            is JSONObject -> v.keys().asSequence().associateWith { v.optString(it) }
            is String -> if (v.isEmpty()) emptyMap() else mapOf("" to v)
            else -> emptyMap()
        }

        /** Текст на языке телефона, иначе английский, иначе любой */
        fun localized(m: Map<String, String>): String {
            if (m.isEmpty()) return ""
            val lang = Locale.getDefault().language
            return m[lang] ?: m["en"] ?: m[""] ?: m.values.first()
        }
    }
}

/** Размер в человеческом виде: 3,0 МБ */
fun formatSize(bytes: Long): String = when {
    bytes <= 0 -> ""
    bytes < 1024 * 1024 -> String.format(Locale.getDefault(), "%.0f KB", bytes / 1024.0)
    else -> String.format(Locale.getDefault(), "%.1f MB", bytes / (1024.0 * 1024.0))
}
