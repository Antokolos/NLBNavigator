// Приложение NLBNavigator. Нативная часть — тот же плеер, что nlb_sdl на ПК (src/main/cpp).
plugins {
    id("com.android.application")
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

    // Варианты: game — самодостаточная игра (книга встроена). Проигрыватель с каталогом — позже
    flavorDimensions += "mode"
    productFlavors {
        create("game") {
            dimension = "mode"
            applicationId = prop("nlb.game.applicationId", "com.nlbhub.nlbgame")
            versionCode = prop("nlb.game.versionCode", "1").toInt()
            versionName = prop("nlb.game.versionName", "1.0")
            buildConfigField("String", "NLB_ARGS", "\"" + prop("nlb.game.args").replace("\"", "\\\"") + "\"")
        }
    }

    buildFeatures {
        buildConfig = true
        resValues = true
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
}
