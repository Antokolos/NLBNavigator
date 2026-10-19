// Приложение NLBNavigator. Нативная часть — тот же плеер, что nlb_sdl на ПК (src/main/cpp).
plugins {
    id("com.android.application")
    id("org.jetbrains.kotlin.plugin.compose")
}

fun prop(name: String, def: String = ""): String = (project.findProperty(name) as String?) ?: def

android {
    namespace = "com.nlbhub.nlbnavigator"
    compileSdk = 36
    ndkVersion = "29.0.14206865"

    defaultConfig {
        minSdk = 25
        targetSdk = 36
        ndk {
            abiFilters.addAll(prop("nlb.abiFilters", "arm64-v8a").split(","))
        }
        externalNativeBuild {
            cmake {
                arguments += listOf("-DANDROID_STL=c++_static")
            }
        }
    }

    // Варианты: game — самодостаточная игра (книга встроена), player — проигрыватель с каталогом
    // (лаунчер на Compose: установленные игры, каталог из интернета, открытие файла)
    flavorDimensions += "mode"
    productFlavors {
        create("player") {
            dimension = "mode"
            applicationId = "com.nlbhub.nlbnavigator"
            versionCode = 1
            versionName = "1.0"
            buildConfigField("String", "NLB_ARGS", "\"\"")
            buildConfigField("String", "NLB_CATALOG_URL", "\"" + prop("nlb.player.catalogUrl") + "\"")
        }
        create("game") {
            dimension = "mode"
            applicationId = prop("nlb.game.applicationId", "com.nlbhub.nlbgame")
            versionCode = prop("nlb.game.versionCode", "1").toInt()
            versionName = prop("nlb.game.versionName", "1.0")
            buildConfigField("String", "NLB_ARGS", "\"" + prop("nlb.game.args").replace("\"", "\\\"") + "\"")
            buildConfigField("String", "NLB_CATALOG_URL", "\"\"")
        }
    }

    buildFeatures {
        buildConfig = true
        resValues = true
        compose = true
    }

    externalNativeBuild {
        cmake {
            path = file("src/main/cpp/CMakeLists.txt")
            version = "3.31.6"
        }
    }

    // Книга сжимать нельзя: SDL читает assets потоком, а сжатые файлы Android отдаёт только целиком
    androidResources {
        noCompress += listOf("nlbpack")
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }
}

dependencies {
    implementation(project(":sdl-activity"))

    // лаунчер — только в проигрывателе
    "playerImplementation"("androidx.activity:activity-compose:1.10.1")
    "playerImplementation"("androidx.compose.ui:ui:1.10.4")
    "playerImplementation"("androidx.compose.foundation:foundation:1.10.4")
    "playerImplementation"("androidx.compose.material3:material3:1.4.0")
    "playerImplementation"("androidx.compose.material:material-icons-core:1.7.8")
    "playerImplementation"("org.jetbrains.kotlinx:kotlinx-coroutines-android:1.11.0")
    "playerImplementation"("io.coil-kt.coil3:coil-compose:3.4.0")
    "playerImplementation"("io.coil-kt.coil3:coil-network-okhttp:3.4.0")
}
