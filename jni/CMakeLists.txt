cmake_minimum_required(VERSION 3.4.1)

find_package(JNI)

if (JNI_FOUND)
    message (STATUS "JNI_INCLUDE_DIRS=${JNI_INCLUDE_DIRS}")
    message (STATUS "JNI_LIBRARIES=${JNI_LIBRARIES}")
else()
    message (STATUS "Can't find JNI")
endif()

include_directories(
        ${CMAKE_SOURCE_DIR}/boofcpp/src/boofcv
        src/main/cpp
        ${JNI_INCLUDE_DIRS})

# build application's shared lib
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11")

add_library(JNIBoofCPP SHARED
        src/main/cpp/JNIBoofCPP.cpp
        src/main/cpp/NativeBlockMean.cpp
        src/main/cpp/NativeGlobalFixed_U8.cpp
        src/main/cpp/NativeGlobalFixed_F32.cpp
        src/main/cpp/NativeGlobalOtsu.cpp)

target_link_libraries(JNIBoofCPP BoofCPP)
