cmake_minimum_required(VERSION 3.31)

project(MyProject LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Read using the v4l API directly.

add_executable(linux-camera src/capture.cpp)

# Use ffmpeg libavdevice and libavcodec.

find_package(PkgConfig REQUIRED)

pkg_check_modules(LIBAV REQUIRED IMPORTED_TARGET
    libavdevice
    libavfilter
    libavformat
    libavcodec
    libswresample
    libswscale
    libavutil
)

add_executable(linux-camera-ffmpeg src/capture_ffmpeg.cpp)
target_link_libraries(linux-camera-ffmpeg PRIVATE PkgConfig::LIBAV)
