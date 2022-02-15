cmake_minimum_required(VERSION 2.8.4)
project(volumio-hook-plugin)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -g -pthread")

set(SOURCE_FILES
    src/pcm_volumiohook.c
    )


# The version number.
set (PCM_VOLUMIO_VERSION_MAJOR 1)
set (PCM_VOLUMIO_VERSION_MINOR 0)

add_definitions(-DPIC)

include_directories(./include)

add_library(asound_module_pcm_volumiohook SHARED ${SOURCE_FILES})
target_link_libraries(asound_module_pcm_volumiohook asound)
