cmake_minimum_required(VERSION 3.16)

find_package(JNI)

if (JNI_FOUND)
    message (STATUS "JNI_INCLUDE_DIRS=${JNI_INCLUDE_DIRS}")
    message (STATUS "JNI_LIBRARIES=${JNI_LIBRARIES}")
else()
    message (STATUS "Can't find JNI")
endif()

# JNI helpers every variant shares: critical arrays, exceptions, enums, configurations and Java images. Compiled into
# each variant's JNI library
file(GLOB JNI_COMMON_SRC "src/main/cpp/*.cpp")

add_library(JNIBoofCPPCommon OBJECT ${JNI_COMMON_SRC})
target_include_directories(JNIBoofCPPCommon PUBLIC src/main/cpp ${JNI_INCLUDE_DIRS})
target_link_libraries(JNIBoofCPPCommon PUBLIC BoofCPPCommon)
set_target_properties(JNIBoofCPPCommon PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Reported by native_build_info()
string(TOUPPER "${CMAKE_BUILD_TYPE}" BOOFCPP_BUILD_TYPE_UPPER)
string(STRIP "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_${BOOFCPP_BUILD_TYPE_UPPER}}" BOOFCPP_CXX_FLAGS)
target_compile_definitions(JNIBoofCPPCommon PRIVATE BOOFCPP_BUILD_TYPE="${CMAKE_BUILD_TYPE}"
        BOOFCPP_CXX_FLAGS="${BOOFCPP_CXX_FLAGS}")

# Test-only JNI library for common's Java tests, e.g. NativeConfigs. Not packaged in any jar
add_library(JNIBoofCPPCommonTest SHARED src/test/cpp/ConfigTestsJNI.cpp)
target_link_libraries(JNIBoofCPPCommonTest BoofCPPCommon JNIBoofCPPCommon)
set_target_properties(JNIBoofCPPCommonTest PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/boofcpp-common-test)
