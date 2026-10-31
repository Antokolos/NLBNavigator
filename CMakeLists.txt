cmake_minimum_required(VERSION 3.16)
project(NLBReader VERSION 1.0.0 LANGUAGES CXX)
# Библиотека может войти в динамический модуль движка (GDExtension, Unreal) — нужен PIC
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Set C++ standard
# C++17 по умолчанию; проект собирается и в C++20 (стандарт модулей Unreal Engine 5):
# cmake -DCMAKE_CXX_STANDARD=20
if(NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 17)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler flags
if(MSVC)
    # /utf-8 — исходники и строковые литералы в UTF-8 (русские сообщения); /EHsc — исключения.
    # Библиотека времени выполнения — по умолчанию CMake (/MD, /MDd): так же собирается Unreal,
    # а статическая (/MT) дала бы конфликт при линковке с ним
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /utf-8 /EHsc")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /utf-8")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")
endif()

# Project structure
set(CPARSE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Source/thirdparty/cparse")
set(NLB_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Source/NLBCore/Public")
set(NLB_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Source/NLBCore/Private")
set(NLB_READER_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Source/NLBReader/Public")
# Слой вывода: снимок экрана, AudioDirector, интерфейсы бэкендов (без внешних зависимостей)
set(NLB_PRESENTATION_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Source/NLBPresentation/Public")
set(NLB_READER_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Source/NLBReader/Private")

# Include directories
include_directories(${NLB_INCLUDE_DIR} ${CPARSE_DIR})

set(CPARSE_HEADERS
        ${CPARSE_DIR}/shunting-yard.h
        ${CPARSE_DIR}/builtin-features.inc
        ${CPARSE_DIR}/packToken.h
        ${CPARSE_DIR}/functions.h
        ${CPARSE_DIR}/containers.h
)

# API Interface Headers
set(NLB_API_HEADERS
        ${NLB_INCLUDE_DIR}/nlb/api/Constants.h
        ${NLB_INCLUDE_DIR}/nlb/api/Coords.h
        ${NLB_INCLUDE_DIR}/nlb/api/CoordsLw.h
        ${NLB_INCLUDE_DIR}/nlb/api/DummyNLB.h
        ${NLB_INCLUDE_DIR}/nlb/api/DummyProgressData.h
        ${NLB_INCLUDE_DIR}/nlb/api/ConsoleProgressData.h
        ${NLB_INCLUDE_DIR}/nlb/api/IdentifiableItem.h
        ${NLB_INCLUDE_DIR}/nlb/api/Link.h
        ${NLB_INCLUDE_DIR}/nlb/api/MediaFile.h
        ${NLB_INCLUDE_DIR}/nlb/api/ModificationsTableModel.h
        ${NLB_INCLUDE_DIR}/nlb/api/Modification.h
        ${NLB_INCLUDE_DIR}/nlb/api/ModifyingItem.h
        ${NLB_INCLUDE_DIR}/nlb/api/NLBCommand.h
        ${NLB_INCLUDE_DIR}/nlb/api/NLBObservable.h
        ${NLB_INCLUDE_DIR}/nlb/api/NLBObserver.h
        ${NLB_INCLUDE_DIR}/nlb/api/NonLinearBook.h
        ${NLB_INCLUDE_DIR}/nlb/api/NodeItem.h
        ${NLB_INCLUDE_DIR}/nlb/api/Page.h
        ${NLB_INCLUDE_DIR}/nlb/api/PartialProgressData.h
        ${NLB_INCLUDE_DIR}/nlb/api/ProgressData.h
        ${NLB_INCLUDE_DIR}/nlb/api/PropertyManager.h
        ${NLB_INCLUDE_DIR}/nlb/api/SearchContract.h
        ${NLB_INCLUDE_DIR}/nlb/api/SearchResultTableModel.h
        ${NLB_INCLUDE_DIR}/nlb/api/TextChunk.h
        ${NLB_INCLUDE_DIR}/nlb/api/Theme.h
        ${NLB_INCLUDE_DIR}/nlb/api/Variable.h
        ${NLB_INCLUDE_DIR}/nlb/api/Obj.h
        ${NLB_INCLUDE_DIR}/nlb/api/Page.h
        ${NLB_INCLUDE_DIR}/nlb/api/NodeItem.h
        ${NLB_INCLUDE_DIR}/nlb/api/Any.h
)

# Domain Headers
set(NLB_DOMAIN_HEADERS
        ${NLB_INCLUDE_DIR}/nlb/domain/AbstractIdentifiableItem.h
        ${NLB_INCLUDE_DIR}/nlb/domain/AbstractNodeItem.h
        ${NLB_INCLUDE_DIR}/nlb/domain/AbstractModifyingItem.h
        ${NLB_INCLUDE_DIR}/nlb/domain/AbstractNodeItem.h
        ${NLB_INCLUDE_DIR}/nlb/domain/Clipboard.h
        ${NLB_INCLUDE_DIR}/nlb/domain/CommandChainCommand.h
        ${NLB_INCLUDE_DIR}/nlb/domain/CoordsImpl.h
        ${NLB_INCLUDE_DIR}/nlb/domain/LinkImpl.h
        ${NLB_INCLUDE_DIR}/nlb/domain/MediaExportParameters.h
        ${NLB_INCLUDE_DIR}/nlb/domain/MediaFileImpl.h
        ${NLB_INCLUDE_DIR}/nlb/domain/ModificationImpl.h
        ${NLB_INCLUDE_DIR}/nlb/domain/NonLinearBookFacade.h
        ${NLB_INCLUDE_DIR}/nlb/domain/NonLinearBookImpl.h
        ${NLB_INCLUDE_DIR}/nlb/domain/NullObj.h
        ${NLB_INCLUDE_DIR}/nlb/domain/ObjImpl.h
        ${NLB_INCLUDE_DIR}/nlb/domain/ObserverHandler.h
        ${NLB_INCLUDE_DIR}/nlb/domain/PageImpl.h
        ${NLB_INCLUDE_DIR}/nlb/domain/SearchResult.h
        ${NLB_INCLUDE_DIR}/nlb/domain/VariableImpl.h
        ${NLB_READER_INCLUDE_DIR}/nlb/domain/NLBReader.h
        ${NLB_READER_INCLUDE_DIR}/nlb/domain/NLBExplorer.h
        ${NLB_READER_INCLUDE_DIR}/nlb/domain/PlayerContext.h
        ${NLB_READER_INCLUDE_DIR}/nlb/domain/ModificationInterpreter.h
        ${NLB_READER_INCLUDE_DIR}/nlb/domain/PlayerEngine.h
        ${NLB_PRESENTATION_INCLUDE_DIR}/nlb/presentation/ScreenModel.h
        ${NLB_PRESENTATION_INCLUDE_DIR}/nlb/presentation/MediaSource.h
        ${NLB_PRESENTATION_INCLUDE_DIR}/nlb/presentation/AudioBackend.h
        ${NLB_PRESENTATION_INCLUDE_DIR}/nlb/presentation/AudioDirector.h
        ${NLB_PRESENTATION_INCLUDE_DIR}/nlb/presentation/BookMediaSource.h
        ${NLB_PRESENTATION_INCLUDE_DIR}/nlb/presentation/Session.h
        ${NLB_PRESENTATION_INCLUDE_DIR}/nlb/presentation/ScreenTheme.h
        ${NLB_PRESENTATION_INCLUDE_DIR}/nlb/presentation/GameScreen.h
)

# Utility Headers
set(NLB_UTIL_HEADERS
        ${NLB_INCLUDE_DIR}/nlb/util/FileManipulator.h
        ${NLB_INCLUDE_DIR}/nlb/util/FileUtils.h
        ${NLB_INCLUDE_DIR}/nlb/util/BookPack.h
        ${NLB_INCLUDE_DIR}/nlb/util/MultiLangString.h
        ${NLB_INCLUDE_DIR}/nlb/util/QuotationHelper.h
        ${NLB_INCLUDE_DIR}/nlb/util/ResourceManager.h
        ${NLB_INCLUDE_DIR}/nlb/util/StringHelper.h
        ${NLB_INCLUDE_DIR}/nlb/util/UUID.h
)

# Exception Headers
set(NLB_EXCEPTION_HEADERS
        ${NLB_INCLUDE_DIR}/nlb/exception/NLBExceptions.h
)

set(CPARSE_SOURCES
        ${CPARSE_DIR}/shunting-yard.cpp
        ${CPARSE_DIR}/packToken.cpp
        ${CPARSE_DIR}/functions.cpp
        ${CPARSE_DIR}/containers.cpp
)

# API Sources
set(NLB_API_SOURCES
        ${NLB_SOURCE_DIR}/nlb/api/Constants.cpp
        ${NLB_SOURCE_DIR}/nlb/api/CoordsLw.cpp
        ${NLB_SOURCE_DIR}/nlb/api/DummyNLB.cpp
        ${NLB_SOURCE_DIR}/nlb/api/ConsoleProgressData.cpp
        ${NLB_SOURCE_DIR}/nlb/api/Modification.cpp
        ${NLB_SOURCE_DIR}/nlb/api/SpecialVariablesNameHelper.cpp
        ${NLB_SOURCE_DIR}/nlb/api/ModificationsTableModel.cpp
        ${NLB_SOURCE_DIR}/nlb/api/PartialProgressData.cpp
        ${NLB_SOURCE_DIR}/nlb/api/PropertyManager.cpp
        ${NLB_SOURCE_DIR}/nlb/api/SearchContract.cpp
        ${NLB_SOURCE_DIR}/nlb/api/SearchResultTableModel.cpp
        ${NLB_SOURCE_DIR}/nlb/api/TextChunk.cpp
        ${NLB_SOURCE_DIR}/nlb/api/Theme.cpp
        ${NLB_SOURCE_DIR}/nlb/api/Variable.cpp
        ${NLB_SOURCE_DIR}/nlb/api/NonLinearBook.cpp
        ${NLB_SOURCE_DIR}/nlb/api/Obj.cpp
        ${NLB_SOURCE_DIR}/nlb/api/Page.cpp
        ${NLB_SOURCE_DIR}/nlb/api/NodeItem.cpp
)

# Domain Sources
set(NLB_DOMAIN_SOURCES
        ${NLB_SOURCE_DIR}/nlb/domain/AbstractIdentifiableItem.cpp
        ${NLB_SOURCE_DIR}/nlb/domain/AbstractNodeItem.cpp
        ${NLB_SOURCE_DIR}/nlb/domain/AbstractModifyingItem.cpp
        ${NLB_SOURCE_DIR}/nlb/domain/AbstractNodeItem.cpp
        ${NLB_SOURCE_DIR}/nlb/domain/Clipboard.cpp
        ${NLB_SOURCE_DIR}/nlb/domain/CommandChainCommand.cpp
        ${NLB_SOURCE_DIR}/nlb/domain/CoordsImpl.cpp
        ${NLB_SOURCE_DIR}/nlb/domain/LinkImpl.cpp
        ${NLB_SOURCE_DIR}/nlb/domain/MediaExportParameters.cpp
        ${NLB_SOURCE_DIR}/nlb/domain/MediaFileImpl.cpp
        ${NLB_SOURCE_DIR}/nlb/domain/ModificationImpl.cpp
        ${NLB_SOURCE_DIR}/nlb/domain/NonLinearBookFacade.cpp
        ${NLB_SOURCE_DIR}/nlb/domain/NonLinearBookImpl.cpp
        ${NLB_SOURCE_DIR}/nlb/domain/NullObj.cpp
        ${NLB_SOURCE_DIR}/nlb/domain/ObjImpl.cpp
        ${NLB_SOURCE_DIR}/nlb/domain/ObserverHandler.cpp
        ${NLB_SOURCE_DIR}/nlb/domain/PageImpl.cpp
        ${NLB_SOURCE_DIR}/nlb/domain/SearchResult.cpp
        ${NLB_SOURCE_DIR}/nlb/domain/VariableImpl.cpp
        ${NLB_READER_SOURCE_DIR}/nlb/domain/NLBReader.cpp
        ${NLB_READER_SOURCE_DIR}/nlb/domain/NLBExplorer.cpp
        ${NLB_READER_SOURCE_DIR}/nlb/domain/NLBExplorerScreenReader.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/NLBPresentation/Private/nlb/presentation/AudioDirector.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/NLBPresentation/Private/nlb/presentation/BookMediaSource.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/NLBPresentation/Private/nlb/presentation/Session.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/NLBPresentation/Private/nlb/presentation/ScreenTheme.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/NLBPresentation/Private/nlb/presentation/GameScreen.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/NLBPresentation/Private/nlb/presentation/ScreenModelJson.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/NLBPresentation/Private/nlb/platform/PlatformBridge.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/NLBPresentation/Private/nlb/presentation/Localization.cpp
        ${NLB_READER_SOURCE_DIR}/nlb/domain/PlayerContext.cpp
        ${NLB_READER_SOURCE_DIR}/nlb/domain/ModificationInterpreter.cpp
        ${NLB_READER_SOURCE_DIR}/nlb/domain/PlayerEngine.cpp
)

# Utility Sources
set(NLB_UTIL_SOURCES
        ${NLB_SOURCE_DIR}/nlb/util/FileManipulator.cpp
        ${NLB_SOURCE_DIR}/nlb/util/FileUtils.cpp
        ${NLB_SOURCE_DIR}/nlb/util/BookPack.cpp
        ${NLB_SOURCE_DIR}/nlb/util/Json.cpp
        ${NLB_SOURCE_DIR}/nlb/util/MultiLangString.cpp
        ${NLB_SOURCE_DIR}/nlb/util/QuotationHelper.cpp
        ${NLB_SOURCE_DIR}/nlb/util/ResourceManager.cpp
        ${NLB_SOURCE_DIR}/nlb/util/StringHelper.cpp
)

# All headers and sources
set(NLB_ALL_HEADERS
        ${NLB_API_HEADERS}
        ${NLB_DOMAIN_HEADERS}
        ${NLB_UTIL_HEADERS}
        ${NLB_EXCEPTION_HEADERS}
)

set(NLB_ALL_SOURCES
        ${NLB_API_SOURCES}
        ${NLB_DOMAIN_SOURCES}
        ${NLB_UTIL_SOURCES}
)

# Create static library
add_library(NLBReader STATIC
        ${NLB_ALL_HEADERS}
        ${NLB_ALL_SOURCES}
)

set_target_properties(NLBReader
        PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
        PUBLIC_HEADER "${NLB_API_HEADERS}"
)

target_include_directories(NLBReader
        PUBLIC
        $<BUILD_INTERFACE:${NLB_INCLUDE_DIR}>
        $<BUILD_INTERFACE:${NLB_READER_INCLUDE_DIR}>
        $<BUILD_INTERFACE:${NLB_PRESENTATION_INCLUDE_DIR}>
        $<INSTALL_INTERFACE:include>
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# std::filesystem отдельной библиотекой нужен только старому GCC (до 9) с libstdc++; clang с libc++
# (Android NDK) и новые GCC её не имеют и не требуют
set(NLB_NEEDS_STDCXXFS OFF)
if(UNIX AND NOT APPLE AND NOT ANDROID AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9)
    set(NLB_NEEDS_STDCXXFS ON)
endif()

# Platform-specific libs
if(WIN32)
    target_link_libraries(NLBReader PRIVATE shlwapi)
elseif(NLB_NEEDS_STDCXXFS)
    target_link_libraries(NLBReader PRIVATE stdc++fs)
endif()

# Build shared library option
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
if(BUILD_SHARED_LIBS)
    add_library(NLBReaderShared SHARED
            ${NLB_ALL_HEADERS}
            ${NLB_ALL_SOURCES}
    )

    set_target_properties(NLBReaderShared PROPERTIES
            VERSION ${PROJECT_VERSION}
            SOVERSION ${PROJECT_VERSION_MAJOR}
            OUTPUT_NAME NLBReader
            PUBLIC_HEADER "${NLB_API_HEADERS}"
    )

    target_include_directories(NLBReaderShared
            PUBLIC
            $<BUILD_INTERFACE:${NLB_INCLUDE_DIR}>
            $<BUILD_INTERFACE:${NLB_READER_INCLUDE_DIR}>
            $<BUILD_INTERFACE:${NLB_PRESENTATION_INCLUDE_DIR}>
            $<INSTALL_INTERFACE:include>
            PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
    )

    if(WIN32)
        target_link_libraries(NLBReaderShared PRIVATE shlwapi)
    elseif(NLB_NEEDS_STDCXXFS)
        target_link_libraries(NLBReaderShared PRIVATE stdc++fs)
    endif()
endif()

# Игровые площадки (достижения, статистика, рекорды): журнал есть всегда; Steam и Epic — если указан
# путь к их SDK (сами SDK в репозиторий не входят: Steamworks — partner.steamgames.com, EOS —
# dev.epicgames.com). Unreal использует OnlineSubsystem, эта библиотека ему не нужна
set(STEAMWORKS_SDK_DIR "" CACHE PATH "Steamworks SDK (папка sdk: public/, redistributable_bin/)")
set(EOS_SDK_DIR "" CACHE PATH "EOS SDK (папка SDK: Include/, Lib/, Bin/)")
add_library(NLBPlatforms STATIC Source/NLBPlatforms/PlatformFactory.h Source/NLBPlatforms/PlatformFactory.cpp)
target_include_directories(NLBPlatforms PUBLIC Source/NLBPlatforms ${CPARSE_DIR})
target_link_libraries(NLBPlatforms PUBLIC NLBReader)
if(STEAMWORKS_SDK_DIR)
    target_sources(NLBPlatforms PRIVATE Source/NLBPlatforms/SteamPlatformServices.h Source/NLBPlatforms/SteamPlatformServices.cpp)
    target_compile_definitions(NLBPlatforms PUBLIC NLB_WITH_STEAMWORKS)
    target_include_directories(NLBPlatforms PUBLIC ${STEAMWORKS_SDK_DIR}/public)
    if(WIN32)
        set(NLB_STEAM_LIB ${STEAMWORKS_SDK_DIR}/redistributable_bin/win64/steam_api64.lib)
        set(NLB_STEAM_DLL ${STEAMWORKS_SDK_DIR}/redistributable_bin/win64/steam_api64.dll)
    elseif(APPLE)
        set(NLB_STEAM_LIB ${STEAMWORKS_SDK_DIR}/redistributable_bin/osx/libsteam_api.dylib)
    else()
        set(NLB_STEAM_LIB ${STEAMWORKS_SDK_DIR}/redistributable_bin/linux64/libsteam_api.so)
    endif()
    target_link_libraries(NLBPlatforms PUBLIC ${NLB_STEAM_LIB})
    message(STATUS "NLBNavigator: Steam (Steamworks SDK) enabled")
endif()
if(EOS_SDK_DIR)
    target_sources(NLBPlatforms PRIVATE Source/NLBPlatforms/EosPlatformServices.h Source/NLBPlatforms/EosPlatformServices.cpp)
    target_compile_definitions(NLBPlatforms PUBLIC NLB_WITH_EOS)
    target_include_directories(NLBPlatforms PUBLIC ${EOS_SDK_DIR}/Include)
    if(WIN32)
        set(NLB_EOS_LIB ${EOS_SDK_DIR}/Lib/EOSSDK-Win64-Shipping.lib)
        set(NLB_EOS_DLL ${EOS_SDK_DIR}/Bin/EOSSDK-Win64-Shipping.dll)
    elseif(APPLE)
        set(NLB_EOS_LIB ${EOS_SDK_DIR}/Bin/libEOSSDK-Mac-Shipping.dylib)
    else()
        set(NLB_EOS_LIB ${EOS_SDK_DIR}/Bin/libEOSSDK-Linux-Shipping.so)
    endif()
    target_link_libraries(NLBPlatforms PUBLIC ${NLB_EOS_LIB})
    message(STATUS "NLBNavigator: Epic Online Services (EOS SDK) enabled")
endif()

# Программы (консоль, SDL3). Встраивание в движки (integrations/godot, Unreal) собирает только
# библиотеку: -DNLB_BUILD_APPS=OFF
option(NLB_BUILD_APPS "Build nlb_main / nlb_sdl executables" ON)
if(NLB_BUILD_APPS)
# Main executable
add_executable(nlb_main
        Source/main.cpp
        Source/PlatformPaths.h
        Source/PlatformPaths.cpp
        Source/ImageConvert.h
        Source/ImageConvert.cpp
        Source/SessionLog.h
        Source/SessionLog.cpp
        ${CPARSE_HEADERS}
        ${CPARSE_SOURCES}
)
target_include_directories(nlb_main
        PUBLIC
        $<BUILD_INTERFACE:${CPARSE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Source/thirdparty>
        $<INSTALL_INTERFACE:include>
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
# Звук через SDL3 + SDL3_mixer (необязательно): консоль и будущий nlb_sdl.
# Ищется автоматически; -DNLB_WITH_SDL3=OFF — собрать без звука
option(NLB_WITH_SDL3 "Build SDL3 audio backend" ON)
if(NLB_WITH_SDL3)
    find_package(SDL3 CONFIG QUIET)
    find_package(SDL3_mixer CONFIG QUIET)
    if(SDL3_FOUND AND SDL3_mixer_FOUND)
        add_library(NLBAudioSDL STATIC
                Source/NLBAudioSDL/Public/nlb/audio/SdlAudioBackend.h
                Source/NLBAudioSDL/Private/nlb/audio/SdlAudioBackend.cpp)
        target_include_directories(NLBAudioSDL PUBLIC Source/NLBAudioSDL/Public)
        target_link_libraries(NLBAudioSDL PUBLIC NLBReader SDL3::SDL3 SDL3_mixer::SDL3_mixer)
        message(STATUS "NLBNavigator: SDL3 audio backend enabled")
        # SDL3_image/SDL3_ttf загружают PNG и FreeType, чьи imported targets ссылаются
        # на ZLIB::ZLIB. Найдём ZLIB явно до загрузки этих конфигураций (в частности, vcpkg).
        find_package(ZLIB REQUIRED)
        # Графический проигрыватель — если есть ещё SDL3_image и SDL3_ttf
        find_package(SDL3_image CONFIG QUIET)
        find_package(SDL3_ttf CONFIG QUIET)
        if(SDL3_image_FOUND AND SDL3_ttf_FOUND)
            add_executable(nlb_sdl
                    Source/NLBSdl/main_sdl.cpp
                    Source/NLBSdl/SdlPlayer.h
                    Source/NLBSdl/SdlPlayer.cpp
                    Source/PlatformPaths.cpp
                    ${CPARSE_HEADERS}
                    ${CPARSE_SOURCES})
            target_include_directories(nlb_sdl PRIVATE Source ${CPARSE_DIR})
            target_link_libraries(nlb_sdl PRIVATE NLBAudioSDL NLBPlatforms SDL3_image::SDL3_image SDL3_ttf::SDL3_ttf)
            # DLL площадок — рядом с программой
            foreach(dll IN ITEMS ${NLB_STEAM_DLL} ${NLB_EOS_DLL})
                add_custom_command(TARGET nlb_sdl POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different ${dll} $<TARGET_FILE_DIR:nlb_sdl>)
            endforeach()
            if(WIN32)
                target_link_libraries(nlb_sdl PRIVATE shell32)
            endif()
            message(STATUS "NLBNavigator: graphical player nlb_sdl enabled")
        endif()
    else()
        message(STATUS "NLBNavigator: SDL3/SDL3_mixer not found — building without sound")
        set(NLB_WITH_SDL3 OFF)
    endif()
endif()

target_link_libraries(nlb_main PRIVATE NLBReader)
if(NLB_WITH_SDL3)
    target_link_libraries(nlb_main PRIVATE NLBAudioSDL)
    target_compile_definitions(nlb_main PRIVATE NLB_WITH_SDL3)
endif()
if(WIN32)
    # CommandLineToArgvW — путь к книге из UTF-16 командной строки
    target_link_libraries(nlb_main PRIVATE shell32)
endif()

endif()  # NLB_BUILD_APPS

# Configuration summary
message(STATUS "")
message(STATUS "Configuration Summary:")
message(STATUS "  Project: ${PROJECT_NAME} ${PROJECT_VERSION}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build shared libs: ${BUILD_SHARED_LIBS}")
message(STATUS "")
