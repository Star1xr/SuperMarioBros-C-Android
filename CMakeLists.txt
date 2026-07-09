cmake_minimum_required(VERSION 3.18)
project(SuperMarioBros-C VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(ANDROID)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    add_library(main SHARED)
    set(TARGET_NAME main)
else()
    add_executable(SuperMarioBros-C)
    set(TARGET_NAME SuperMarioBros-C)
endif()

set(SDL2_DISABLE_SDL2MAIN ON CACHE BOOL "")
set(SDL2_DISABLE_INSTALL ON CACHE BOOL "")
add_subdirectory(${CMAKE_SOURCE_DIR}/SDL2)

target_sources(${TARGET_NAME} PRIVATE
    source/Main.cpp
    source/Configuration.cpp
    source/OpenGLRendering.cpp
    source/TouchControls.cpp
    source/Emulation/APU.cpp
    source/Emulation/Controller.cpp
    source/Emulation/MemoryAccess.cpp
    source/Emulation/PPU.cpp
    source/SMB/SMB.cpp
    source/SMB/SMBData.cpp
    source/SMB/SMBEngine.cpp
    source/Util/Video.cpp
)

target_include_directories(${TARGET_NAME} PRIVATE source)
target_compile_options(${TARGET_NAME} PRIVATE -Wno-c++11-narrowing)

if(ANDROID)
    target_link_libraries(${TARGET_NAME} PRIVATE SDL2::SDL2 GLESv2)
else()
    target_link_libraries(${TARGET_NAME} PRIVATE SDL2::SDL2 opengl32)
endif()
