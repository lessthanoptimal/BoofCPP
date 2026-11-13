cmake_minimum_required(VERSION 3.16)

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

file(GLOB JNI_SRC "src/main/cpp/*.cpp")

add_library(JNIBoofCPP SHARED ${JNI_SRC})

target_link_libraries(JNIBoofCPP BoofCPP)
