cmake_minimum_required(VERSION 3.22.1)

project("andffme")

add_library( # Sets the name of the library.
        andffme
        SHARED
        RtmpPusher.cpp
        AudioStream.cpp
        VideoStream.cpp
        day7.cpp
        native-lib.cpp)


set(IMPORT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../jniLibs)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/rtmp)


include_directories(${IMPORT_DIR}/${ANDROID_ABI}/include)


#include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(faac
        STATIC
        IMPORTED)
set_target_properties(faac
        PROPERTIES IMPORTED_LOCATION
        ${IMPORT_DIR}/${ANDROID_ABI}/libfaac.a)

add_library(x264
        STATIC
        IMPORTED)
set_target_properties(x264
        PROPERTIES IMPORTED_LOCATION
        ${IMPORT_DIR}/${ANDROID_ABI}/libx264.a)


# Codec library
add_library(
        avcodec
        SHARED
        IMPORTED
)
set_target_properties(
        avcodec
        PROPERTIES IMPORTED_LOCATION
        ${IMPORT_DIR}/${ANDROID_ABI}/libavcodec.so
)
# The filter library is temporarily out of use
add_library(
        avfilter
        SHARED
        IMPORTED
)
set_target_properties(
        avfilter
        PROPERTIES IMPORTED_LOCATION
        ${IMPORT_DIR}/${ANDROID_ABI}/libavfilter.so
)

# File format libraries are required for most operations
add_library(
        avformat
        SHARED
        IMPORTED
)

set_target_properties(
        avformat
        PROPERTIES IMPORTED_LOCATION
        ${IMPORT_DIR}/${ANDROID_ABI}/libavformat.so
)

# Tool library
add_library(
        avutil
        SHARED
        IMPORTED
)
set_target_properties(
        avutil
        PROPERTIES IMPORTED_LOCATION
        ${IMPORT_DIR}/${ANDROID_ABI}/libavutil.so
)

# The resampling library is mainly used for audio conversion.
add_library(
        swresample
        SHARED
        IMPORTED
)
set_target_properties(
        swresample
        PROPERTIES IMPORTED_LOCATION
        ${IMPORT_DIR}/${ANDROID_ABI}/libswresample.so
)

# Video format conversion library is mainly used for video conversion.
add_library(
        swscale
        SHARED
        IMPORTED
)
set_target_properties(
        swscale
        PROPERTIES IMPORTED_LOCATION
        ${IMPORT_DIR}/${ANDROID_ABI}/libswscale.so
)

find_library( log-lib
        log )

# The main android library, native window, requires this library
target_link_libraries(
        andffme
        ${log-lib}
        x264
        faac
        rtmp
        android
        avcodec
        avfilter
        avformat
        avutil
        swresample
        swscale
)