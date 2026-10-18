// Java-часть SDL3 (org.libsdl.app.SDLActivity и др.) — как есть из SDL release-3.4.18,
// лицензия zlib (LICENSE-SDL.txt)
plugins {
    id("com.android.library")
}

android {
    namespace = "org.libsdl.app"
    compileSdk = 36
    defaultConfig {
        minSdk = 25
    }
    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }
}
