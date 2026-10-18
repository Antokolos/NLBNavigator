# Нативная часть Android: SDL3 и библиотеки к нему из исходников (android/deps, скрипт
# fetch_deps.sh) и плеер — те же исходники, что nlb_sdl на ПК. Результат — libmain.so
# (SDLActivity загружает libSDL3.so и libmain.so и вызывает SDL_main).
cmake_minimum_required(VERSION 3.22)
project(nlbnavigator_android C CXX)

# pkg-config с ПК (например, из MSYS2 в PATH) для Android не годится: он нашёл бы библиотеки
# Windows. Всё нужное собирается из исходников, поэтому pkg-config не ищем
set(CMAKE_DISABLE_FIND_PACKAGE_PkgConfig ON)

get_filename_component(NLB_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../../.." ABSOLUTE)
set(DEPS "${NLB_ROOT}/android/deps")
if(NOT EXISTS "${DEPS}/SDL/CMakeLists.txt")
    message(FATAL_ERROR "Нет исходников SDL3: запустите android/fetch_deps.sh")
endif()

# ---- SDL3 (разделяемая: её Java-часть загружает libSDL3.so)
set(SDL_SHARED ON CACHE BOOL "" FORCE)
set(SDL_STATIC OFF CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory("${DEPS}/SDL" SDL)

# ---- SDL_image, SDL_ttf, SDL_mixer — статически внутри libmain.so, без внешних библиотек:
# картинки — stb_image, шрифты — встроенный FreeType, звук — stb_vorbis, dr_mp3, dr_flac
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

set(SDLIMAGE_VENDORED OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_SAMPLES OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_TESTS OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_INSTALL OFF CACHE BOOL "" FORCE)
foreach(f AVIF JXL TIF WEBP)
    set(SDLIMAGE_${f} OFF CACHE BOOL "" FORCE)
endforeach()
add_subdirectory("${DEPS}/SDL_image" SDL_image)

set(SDLTTF_VENDORED ON CACHE BOOL "" FORCE)
set(SDLTTF_HARFBUZZ OFF CACHE BOOL "" FORCE)
set(SDLTTF_PLUTOSVG OFF CACHE BOOL "" FORCE)
set(SDLTTF_SAMPLES OFF CACHE BOOL "" FORCE)
set(SDLTTF_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory("${DEPS}/SDL_ttf" SDL_ttf)

set(SDLMIXER_VENDORED OFF CACHE BOOL "" FORCE)
set(SDLMIXER_DEPS_SHARED OFF CACHE BOOL "" FORCE)
set(SDLMIXER_TESTS OFF CACHE BOOL "" FORCE)
set(SDLMIXER_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SDLMIXER_INSTALL OFF CACHE BOOL "" FORCE)
foreach(f FLAC_LIBFLAC GME MOD MP3_MPG123 MIDI OPUS VORBIS_VORBISFILE WAVPACK)
    set(SDLMIXER_${f} OFF CACHE BOOL "" FORCE)
endforeach()
add_subdirectory("${DEPS}/SDL_mixer" SDL_mixer)

# ---- Плеер: библиотека NLB (без программ) и nlb_sdl
set(NLB_BUILD_APPS OFF CACHE BOOL "" FORCE)
add_subdirectory("${NLB_ROOT}" nlb)

add_library(main SHARED
        "${NLB_ROOT}/Source/NLBSdl/main_sdl.cpp"
        "${NLB_ROOT}/Source/NLBSdl/SdlPlayer.cpp"
        "${NLB_ROOT}/Source/NLBAudioSDL/Private/nlb/audio/SdlAudioBackend.cpp"
        "${NLB_ROOT}/Source/PlatformPaths.cpp"
        # интерпретатор выражений cparse — как в программах на ПК, отдельно от библиотеки NLBReader
        "${NLB_ROOT}/Source/thirdparty/cparse/shunting-yard.cpp"
        "${NLB_ROOT}/Source/thirdparty/cparse/packToken.cpp"
        "${NLB_ROOT}/Source/thirdparty/cparse/functions.cpp"
        "${NLB_ROOT}/Source/thirdparty/cparse/containers.cpp")
target_include_directories(main PRIVATE
        "${NLB_ROOT}/Source"
        "${NLB_ROOT}/Source/NLBSdl"
        "${NLB_ROOT}/Source/NLBAudioSDL/Public")
target_compile_features(main PRIVATE cxx_std_20)
target_include_directories(main PRIVATE "${NLB_ROOT}/Source/thirdparty/cparse")
# все символы должны найтись при сборке (как требует Android) — и при проверке на ПК
target_link_options(main PRIVATE "-Wl,--no-undefined")
target_link_libraries(main PRIVATE
        NLBReader NLBPlatforms
        SDL3::SDL3-shared SDL3_image::SDL3_image-static SDL3_ttf::SDL3_ttf-static SDL3_mixer::SDL3_mixer-static)
if(ANDROID)
    target_link_libraries(main PRIVATE android log)
endif()
