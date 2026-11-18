cmake_minimum_required(VERSION 3.16)

file(GLOB JNI_FAST_SRC "src/main/cpp/*.cpp")

add_library(JNIBoofCPPFast SHARED ${JNI_FAST_SRC})
target_link_libraries(JNIBoofCPPFast BoofCPPFastObjects BoofCPPObjects BoofCPPCommon JNIBoofCPPCommon)
# Strict's JNI helpers for the hooks fast replaces, e.g. ThresholdJNI.h
target_include_directories(JNIBoofCPPFast PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../strict/jni/src/main/cpp)
# The Java code and the native jar load it from build/boofcpp-fast-jni
set_target_properties(JNIBoofCPPFast PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/boofcpp-fast-jni)
