// NLBNavigator для Android: плеер книг NLB на SDL3.
//   app          — приложение (варианты сборки: game — самодостаточная игра со встроенной книгой)
//   sdl-activity — Java-часть SDL3 (SDLActivity), из SDL release-3.4.18
pluginManagement {
    repositories {
        google()
        mavenCentral()
        gradlePluginPortal()
    }
}

dependencyResolutionManagement {
    repositories {
        google()
        mavenCentral()
    }
}

rootProject.name = "NLBNavigator"
include(":app")
include(":sdl-activity")
