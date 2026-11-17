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
