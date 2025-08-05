cmake_minimum_required(VERSION 3.16)
project(NLBReader VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler flags
if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT /O2")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")
endif()

# Project structure
set(NLB_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Source/NLBCore/Public")
set(NLB_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Source/NLBCore/Private")

# Include directories
include_directories(${NLB_INCLUDE_DIR})

# API Interface Headers
set(NLB_API_HEADERS
        ${NLB_INCLUDE_DIR}/nlb/api/Constants.h
        ${NLB_INCLUDE_DIR}/nlb/api/Coords.h
        ${NLB_INCLUDE_DIR}/nlb/api/CoordsLw.h
        ${NLB_INCLUDE_DIR}/nlb/api/DummyNLB.h
        ${NLB_INCLUDE_DIR}/nlb/api/IdentifiableItem.h
        ${NLB_INCLUDE_DIR}/nlb/api/Link.h
        ${NLB_INCLUDE_DIR}/nlb/api/MediaFile.h
        ${NLB_INCLUDE_DIR}/nlb/api/ModificationsTableModel.h
        ${NLB_INCLUDE_DIR}/nlb/api/Modification.h
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
)

# Utility Headers
set(NLB_UTIL_HEADERS
        ${NLB_INCLUDE_DIR}/nlb/util/FileManipulator.h
        ${NLB_INCLUDE_DIR}/nlb/util/FileUtils.h
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

# API Sources
set(NLB_API_SOURCES
        ${NLB_SOURCE_DIR}/nlb/api/Constants.cpp
        ${NLB_SOURCE_DIR}/nlb/api/CoordsLw.cpp
        ${NLB_SOURCE_DIR}/nlb/api/DummyNLB.cpp
        ${NLB_SOURCE_DIR}/nlb/api/Modification.cpp
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
)

# Utility Sources
set(NLB_UTIL_SOURCES
        ${NLB_SOURCE_DIR}/nlb/util/FileManipulator.cpp
        ${NLB_SOURCE_DIR}/nlb/util/FileUtils.cpp
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
        $<INSTALL_INTERFACE:include>
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Platform-specific libs
if(WIN32)
    target_link_libraries(NLBReader PRIVATE shlwapi)
elseif(UNIX AND NOT APPLE)
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
            $<INSTALL_INTERFACE:include>
            PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
    )

    if(WIN32)
        target_link_libraries(NLBReaderShared PRIVATE shlwapi)
    elseif(UNIX AND NOT APPLE)
        target_link_libraries(NLBReaderShared PRIVATE stdc++fs)
    endif()
endif()

# Main executable
add_executable(nlb_main
        Source/main.cpp
)
target_link_libraries(nlb_main PRIVATE NLBReader)

# Configuration summary
message(STATUS "")
message(STATUS "Configuration Summary:")
message(STATUS "  Project: ${PROJECT_NAME} ${PROJECT_VERSION}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build shared libs: ${BUILD_SHARED_LIBS}")
message(STATUS "")