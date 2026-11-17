cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_FLAGS_RELEASE "-O3")

# Bit identical results with Java: never fuse multiply-add (arm64 has FMA)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffp-contract=off")

set(path_to_libs ${CMAKE_CURRENT_SOURCE_DIR}/../libs)
set(path_to_jni ${path_to_libs}/strict/jni/src/main/cpp)
set(path_to_common_jni ${path_to_libs}/common/jni/src/main/cpp)

file(GLOB BOOFCPP_SRCS ${path_to_libs}/common/cpp/src/*.cpp ${path_to_libs}/strict/cpp/src/*.cpp)
file(GLOB BOOFCPP_HDRS ${path_to_libs}/common/cpp/include/boofcv/*.h ${path_to_libs}/strict/cpp/include/boofcv/*.h)

file(GLOB JNI_SRCS ${path_to_common_jni}/*.cpp ${path_to_jni}/*.cpp)
file(GLOB JNI_HDRS ${path_to_common_jni}/*.h ${path_to_jni}/*.h)

include_directories( ${path_to_libs}/common/cpp/include ${path_to_libs}/strict/cpp/include ${path_to_common_jni} ${path_to_jni} )

#MESSAGE( STATUS "path_to_quirc_detector:         " ${path_to_quirc_detector} )


add_library(JNIBoofCPP SHARED
        ${BOOFCPP_SRCS} ${BOOFCPP_HDRS}
        ${JNI_SRCS} ${JNI_HDRS})


find_library( # Sets the name of the path variable.
              log-lib

              # Specifies the name of the NDK library that
              # you want CMake to locate.
              log )

target_link_libraries( # Specifies the target library.
                       JNIBoofCPP

                       # Links the target library to the log library
                       # included in the NDK.
                       ${log-lib} )