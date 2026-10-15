# GDExtension NLBNavigator для Godot 4.4+.
#   cmake -B build -DGODOT_CPP_DIR=<путь к godot-cpp (ветка godot-4.4-stable)> -DCMAKE_BUILD_TYPE=Release
#   cmake --build build
# Результат (nlbnavigator.dll / libnlbnavigator.so) кладётся в demo/bin — рядом с nlbnavigator.gdextension
cmake_minimum_required(VERSION 3.22)
project(nlbnavigator_godot LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(GODOT_CPP_DIR "" CACHE PATH "godot-cpp checkout (godot-4.4-stable)")
if(NOT GODOT_CPP_DIR)
    message(FATAL_ERROR "Укажите -DGODOT_CPP_DIR=<путь к godot-cpp>")
endif()
add_subdirectory(${GODOT_CPP_DIR} godot-cpp)

# Библиотека плеера без программ
set(NLB_BUILD_APPS OFF CACHE BOOL "" FORCE)
set(NLB_WITH_SDL3 OFF CACHE BOOL "" FORCE)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../.. nlbnavigator)

set(CPARSE "${CMAKE_CURRENT_SOURCE_DIR}/../../Source/thirdparty/cparse")
add_library(nlbnavigator SHARED
        src/register_types.cpp
        src/nlb_player.h
        src/nlb_player.cpp
        src/godot_audio_backend.h
        src/godot_audio_backend.cpp
        ${CPARSE}/shunting-yard.cpp
        ${CPARSE}/packToken.cpp
        ${CPARSE}/functions.cpp
        ${CPARSE}/containers.cpp)
target_include_directories(nlbnavigator PRIVATE src ${CPARSE})
# godot-cpp 4.4: цели godot-cpp::template_debug / template_release / editor
set(NLB_GODOT_TARGET "template_release" CACHE STRING "godot-cpp target: template_debug, template_release or editor")
target_link_libraries(nlbnavigator PRIVATE godot-cpp::${NLB_GODOT_TARGET} NLBReader)
# godot-cpp передаёт зависящим целям -fno-exceptions, а плеер и cparse используют исключения.
# Включаем их обратно для файлов модуля свойством самих файлов: оно идёт в командной строке
# после флагов зависимостей (флаги цели — до них и проигрывают). Сам godot-cpp исключений не бросает
get_target_property(NLB_GODOT_SOURCES nlbnavigator SOURCES)
if(MSVC)
    set_source_files_properties(${NLB_GODOT_SOURCES} PROPERTIES COMPILE_OPTIONS "/EHsc" COMPILE_DEFINITIONS "_HAS_EXCEPTIONS=1")
else()
    set_source_files_properties(${NLB_GODOT_SOURCES} PROPERTIES COMPILE_OPTIONS "-fexceptions")
endif()
set_target_properties(nlbnavigator PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/demo/bin"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/demo/bin"
        PREFIX "")
if(MINGW)
    # MinGW: стандартная библиотека внутрь модуля, чтобы Godot не искал её DLL
    target_link_options(nlbnavigator PRIVATE -static-libgcc -static-libstdc++)
endif()
