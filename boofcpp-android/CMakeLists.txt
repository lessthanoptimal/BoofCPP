cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_FLAGS_RELEASE "-O3")

set(path_to_boofcpp ${CMAKE_CURRENT_SOURCE_DIR}/../boofcpp/src/boofcv)
set(path_to_jni ${CMAKE_CURRENT_SOURCE_DIR}/../boofcpp-jni/src/main/cpp)

file(GLOB_RECURSE BOOFCPP_SRCS ${path_to_boofcpp}/*.cpp)
file(GLOB_RECURSE BOOFCPP_HDRS ${path_to_boofcpp}/*.h)

file(GLOB_RECURSE JNI_SRCS ${path_to_jni}/*.cpp)
file(GLOB_RECURSE JNI_HDRS ${path_to_jni}/*.h)

include_directories( ${path_to_boofcpp} )

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