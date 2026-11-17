cmake_minimum_required(VERSION 3.16)

file(GLOB JNI_FAST_SRC "src/main/cpp/*.cpp")

add_library(JNIBoofCPPFast SHARED ${JNI_FAST_SRC})
target_link_libraries(JNIBoofCPPFast BoofCPPFastObjects BoofCPPCommon JNIBoofCPPCommon)
# The Java code and the native jar load it from build/boofcpp-fast-jni
set_target_properties(JNIBoofCPPFast PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/boofcpp-fast-jni)
