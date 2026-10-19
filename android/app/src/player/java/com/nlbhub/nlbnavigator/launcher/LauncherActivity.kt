package com.nlbhub.nlbnavigator.launcher

import android.content.Intent
import android.net.Uri
import android.os.Bundle
import android.provider.OpenableColumns
import androidx.activity.ComponentActivity
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.compose.setContent
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Add
import androidx.compose.material.icons.filled.Refresh
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Button
import androidx.compose.material3.Card
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.ExtendedFloatingActionButton
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.LinearProgressIndicator
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.Scaffold
import androidx.compose.material3.SnackbarHost
import androidx.compose.material3.SnackbarHostState
import androidx.compose.material3.Tab
import androidx.compose.material3.TabRow
import androidx.compose.material3.Text
import androidx.compose.material3.TopAppBar
import androidx.compose.material3.darkColorScheme
import androidx.compose.material3.lightColorScheme
import androidx.compose.foundation.isSystemInDarkTheme
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.layout.ContentScale
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import coil3.compose.AsyncImage
import com.nlbhub.nlbnavigator.BuildConfig
import com.nlbhub.nlbnavigator.NLBActivity
import com.nlbhub.nlbnavigator.R
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext

/**
 * Лаунчер проигрывателя: «Установленные» (играть, удалить), «Каталог» (скачать, обновить
 * каталог) и «Открыть файл» (.nlbpack или .nlbz с устройства). Игра запускается в NLBActivity.
 */
class LauncherActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        val library = Library(this)
        val catalog = Catalog(this, BuildConfig.NLB_CATALOG_URL)
        setContent {
            val colors = if (isSystemInDarkTheme()) darkColorScheme() else lightColorScheme()
            MaterialTheme(colorScheme = colors) {
                LauncherScreen(library, catalog, ::play)
            }
        }
    }

    private fun play(game: Library.Installed) {
        startActivity(Intent(this, NLBActivity::class.java)
            .putExtra(NLBActivity.EXTRA_BOOK, game.book.absolutePath)
            .putExtra(NLBActivity.EXTRA_ARGS, game.info.playerArgs()))
    }
}

@OptIn(ExperimentalMaterial3Api::class)
@Composable
private fun LauncherScreen(library: Library, catalog: Catalog, play: (Library.Installed) -> Unit) {
    val scope = rememberCoroutineScope()
    val snackbar = remember { SnackbarHostState() }
    val context = androidx.compose.ui.platform.LocalContext.current
    var tab by remember { mutableStateOf(0) }
    var installed by remember { mutableStateOf(library.list()) }
    var games by remember { mutableStateOf(runCatching { catalog.load() }.getOrDefault(emptyList())) }
    var busy by remember { mutableStateOf<String?>(null) }      // что сейчас ставится
    var progress by remember { mutableStateOf(0f) }
    var details by remember { mutableStateOf<GameInfo?>(null) }  // карточка игры из каталога
    var toDelete by remember { mutableStateOf<Library.Installed?>(null) }
    val errorText = stringResource(R.string.error, "%s")
    val updatedText = stringResource(R.string.catalog_updated)

    fun install(label: String, size: Long, action: ((Long) -> Unit) -> GameInfo) {
        busy = label
        progress = 0f
        scope.launch {
            val result = withContext(Dispatchers.IO) {
                runCatching { action { done -> if (size > 0) progress = (done.toFloat() / size).coerceIn(0f, 1f) } }
            }
            busy = null
            installed = library.list()
            result.onSuccess { tab = 0 }
            result.onFailure { snackbar.showSnackbar(errorText.replace("%s", it.message ?: it.toString())) }
        }
    }

    val openFile = rememberLauncherForActivityResult(ActivityResultContracts.OpenDocument()) { uri: Uri? ->
        if (uri != null) {
            var name = "game.nlbpack"
            var size = 0L
            context.contentResolver.query(uri, null, null, null, null)?.use { c ->
                if (c.moveToFirst()) {
                    val n = c.getColumnIndex(OpenableColumns.DISPLAY_NAME)
                    val s = c.getColumnIndex(OpenableColumns.SIZE)
                    if (n >= 0) name = c.getString(n) ?: name
                    if (s >= 0) size = c.getLong(s)
                }
            }
            install(name, size) { p -> library.installFromUri(uri, name, p) }
        }
    }

    Scaffold(
        topBar = {
            TopAppBar(
                title = { Text(stringResource(R.string.app_name)) },
                actions = {
                    if (tab == 1) {
                        IconButton(onClick = {
                            scope.launch {
                                val r = withContext(Dispatchers.IO) { runCatching { catalog.refresh() } }
                                r.onSuccess { games = it; snackbar.showSnackbar(updatedText) }
                                r.onFailure { snackbar.showSnackbar(errorText.replace("%s", it.message ?: it.toString())) }
                            }
                        }) { Icon(Icons.Filled.Refresh, contentDescription = stringResource(R.string.refresh)) }
                    }
                },
            )
        },
        floatingActionButton = {
            ExtendedFloatingActionButton(
                onClick = { openFile.launch(arrayOf("*/*")) },
                icon = { Icon(Icons.Filled.Add, contentDescription = null) },
                text = { Text(stringResource(R.string.open_file)) },
            )
        },
        snackbarHost = { SnackbarHost(snackbar) },
    ) { padding ->
        Column(Modifier.padding(padding).fillMaxSize()) {
            TabRow(selectedTabIndex = tab) {
                Tab(selected = tab == 0, onClick = { tab = 0 }, text = { Text(stringResource(R.string.tab_installed)) })
                Tab(selected = tab == 1, onClick = { tab = 1 }, text = { Text(stringResource(R.string.tab_catalog)) })
            }
            busy?.let { label ->
                Column(Modifier.fillMaxWidth().padding(16.dp)) {
                    Text(stringResource(R.string.installing) + " " + label, style = MaterialTheme.typography.bodyMedium)
                    Spacer(Modifier.height(8.dp))
                    if (progress > 0f) LinearProgressIndicator(progress = { progress }, modifier = Modifier.fillMaxWidth())
                    else LinearProgressIndicator(modifier = Modifier.fillMaxWidth())
                }
            }
            if (tab == 0) {
                if (installed.isEmpty()) EmptyText(stringResource(R.string.installed_empty))
                LazyColumn(contentPadding = androidx.compose.foundation.layout.PaddingValues(12.dp, 8.dp, 12.dp, 96.dp),
                           verticalArrangement = Arrangement.spacedBy(8.dp)) {
                    items(installed, key = { it.info.id }) { game ->
                        GameCard(game.info, onClick = { play(game) }) {
                            Button(onClick = { play(game) }) { Text(stringResource(R.string.play)) }
                            Spacer(Modifier.width(8.dp))
                            OutlinedButton(onClick = { toDelete = game }) { Text(stringResource(R.string.delete)) }
                        }
                    }
                }
            } else {
                if (games.isEmpty()) EmptyText(stringResource(R.string.catalog_empty))
                LazyColumn(contentPadding = androidx.compose.foundation.layout.PaddingValues(12.dp, 8.dp, 12.dp, 96.dp),
                           verticalArrangement = Arrangement.spacedBy(8.dp)) {
                    items(games, key = { it.id }) { game ->
                        GameCard(game, onClick = { details = game }) {
                            InstallButton(game, installed, busy == null) { install(game.title(), game.size) { p -> library.installFromCatalog(game, p) } }
                        }
                    }
                }
            }
        }
    }

    details?.let { game ->
        AlertDialog(
            onDismissRequest = { details = null },
            title = { Text(game.title()) },
            text = {
                Column(Modifier.verticalScroll(rememberScrollState())) {
                    if (game.screenshot.isNotEmpty()) {
                        AsyncImage(model = game.screenshot, contentDescription = null, contentScale = ContentScale.FillWidth,
                                   modifier = Modifier.fillMaxWidth())
                        Spacer(Modifier.height(8.dp))
                    }
                    Meta(game)
                    Spacer(Modifier.height(8.dp))
                    Text(game.description())
                }
            },
            confirmButton = {
                InstallButton(game, installed, busy == null) {
                    details = null
                    install(game.title(), game.size) { p -> library.installFromCatalog(game, p) }
                }
            },
            dismissButton = { OutlinedButton(onClick = { details = null }) { Text(stringResource(R.string.cancel)) } },
        )
    }

    toDelete?.let { game ->
        AlertDialog(
            onDismissRequest = { toDelete = null },
            title = { Text(stringResource(R.string.delete) + ": " + game.info.title() + "?") },
            confirmButton = {
                Button(onClick = { library.delete(game.info.id); installed = library.list(); toDelete = null }) {
                    Text(stringResource(R.string.delete))
                }
            },
            dismissButton = { OutlinedButton(onClick = { toDelete = null }) { Text(stringResource(R.string.cancel)) } },
        )
    }
}

/** «Установить», «Обновить» (в каталоге версия новее) или «Установлена: версия» */
@Composable
private fun InstallButton(game: GameInfo, installed: List<Library.Installed>, enabled: Boolean, onInstall: () -> Unit) {
    val have = installed.firstOrNull { it.info.id == game.id }
    when {
        have == null -> Button(onClick = onInstall, enabled = enabled && game.url.isNotEmpty()) { Text(stringResource(R.string.install)) }
        have.info.version != game.version -> Button(onClick = onInstall, enabled = enabled && game.url.isNotEmpty()) { Text(stringResource(R.string.update)) }
        else -> Text(stringResource(R.string.installed_label, have.info.version), style = MaterialTheme.typography.bodySmall)
    }
}

@Composable
private fun GameCard(game: GameInfo, onClick: () -> Unit, buttons: @Composable () -> Unit) {
    Card(Modifier.fillMaxWidth().clickable(onClick = onClick)) {
        Row(Modifier.padding(12.dp)) {
            if (game.thumbnail.isNotEmpty()) {
                AsyncImage(model = game.thumbnail, contentDescription = null, contentScale = ContentScale.Crop,
                           modifier = Modifier.size(72.dp))
                Spacer(Modifier.width(12.dp))
            }
            Column(Modifier.weight(1f)) {
                Text(game.title(), style = MaterialTheme.typography.titleMedium, maxLines = 2, overflow = TextOverflow.Ellipsis)
                Meta(game)
                if (game.summary().isNotEmpty()) {
                    Text(game.summary(), style = MaterialTheme.typography.bodySmall, maxLines = 3, overflow = TextOverflow.Ellipsis)
                }
                Spacer(Modifier.height(8.dp))
                Row(verticalAlignment = Alignment.CenterVertically) { buttons() }
            }
        }
    }
}

@Composable
private fun Meta(game: GameInfo) {
    val parts = listOfNotNull(
        game.author().takeIf { it.isNotEmpty() }?.let { stringResource(R.string.author, it) },
        game.version.takeIf { it.isNotEmpty() }?.let { stringResource(R.string.version, it) },
        formatSize(game.size).takeIf { it.isNotEmpty() },
    )
    if (parts.isNotEmpty()) Text(parts.joinToString(" · "), style = MaterialTheme.typography.bodySmall)
}

@Composable
private fun EmptyText(text: String) {
    Box(Modifier.fillMaxWidth().padding(24.dp), contentAlignment = Alignment.Center) {
        Text(text, style = MaterialTheme.typography.bodyMedium)
    }
}
