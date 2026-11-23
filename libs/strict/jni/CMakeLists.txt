cmake_minimum_required(VERSION 3.16)

include_directories(src/main/cpp)

# build application's shared lib

file(GLOB JNI_SRC "src/main/cpp/*.cpp")

add_library(JNIBoofCPP SHARED ${JNI_SRC})

target_link_libraries(JNIBoofCPP BoofCPPObjects JNIBoofCPPCommon)
# The Java code and the native jar load it from build/boofcpp-jni
set_target_properties(JNIBoofCPP PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/boofcpp-jni)
