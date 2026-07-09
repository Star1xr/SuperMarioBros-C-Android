cmake_minimum_required(VERSION 3.18)
project(SuperMarioBros-C VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(ANDROID)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    add_library(main SHARED)
    include_directories(${CMAKE_SOURCE_DIR}/SDL2/include)
else()
    add_executable(SuperMarioBros-C)
endif()

target_sources(SuperMarioBros-C PRIVATE
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

target_include_directories(SuperMarioBros-C PRIVATE source)

if(ANDROID)
    target_link_libraries(main PRIVATE SDL2-static GLESv2)
else()
    target_link_libraries(SuperMarioBros-C PRIVATE SDL2-static opengl32)
endif()
