package com.nlbhub.nlbnavigator.launcher

import android.content.Context
import org.json.JSONObject
import java.io.File
import java.io.IOException
import java.net.HttpURLConnection
import java.net.URL

/**
 * Каталог игр: JSON с сервера (адрес — nlb.player.catalogUrl в gradle.properties). Хранится в
 * кэше приложения и заново скачивается по кнопке «Обновить»; до первого обновления — встроенный
 * в приложение (assets/catalog.json).
 *
 * {"games": [ {id, version, title{язык: …}, author{…}, summary{…}, description{…}, mode, args,
 *              size, url, thumbnail, screenshot}, … ]}
 */
class Catalog(private val context: Context, private val url: String) {
    private val cache = File(context.filesDir, "catalog.json")

    fun load(): List<GameInfo> {
        val text = if (cache.isFile) cache.readText()
        else context.assets.open("catalog.json").use { it.readBytes().toString(Charsets.UTF_8) }
        return parse(text)
    }

    /** Скачать каталог заново; при ошибке старый остаётся */
    fun refresh(): List<GameInfo> {
        if (url.isBlank()) throw IOException("адрес каталога не задан")
        val conn = URL(url).openConnection() as HttpURLConnection
        conn.connectTimeout = 15000
        conn.readTimeout = 30000
        try {
            if (conn.responseCode !in 200..299) throw IOException("HTTP ${conn.responseCode}")
            val text = conn.inputStream.use { it.readBytes().toString(Charsets.UTF_8) }
            val games = parse(text)  // сначала проверить, что это каталог
            cache.writeText(text)
            return games
        } finally {
            conn.disconnect()
        }
    }

    private fun parse(text: String): List<GameInfo> {
        val array = JSONObject(text).optJSONArray("games") ?: return emptyList()
        return (0 until array.length()).map { GameInfo.fromJson(array.getJSONObject(it)) }.filter { it.id.isNotBlank() }
    }
}
