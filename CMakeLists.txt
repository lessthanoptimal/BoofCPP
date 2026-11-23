cmake_minimum_required(VERSION 3.16)
project(boofcpp_all)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

#if(NOT CMAKE_BUILD_TYPE)
#    set(CMAKE_BUILD_TYPE Release)
#endif()
#
#set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

# Results must be bit identical to Java, which never fuses multiply and add. Without this, compilers targeting
# hardware with FMA (e.g. arm64 / Android, or -march=native) can contract a*b+c and change floating point results.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffp-contract=off")

# SSE4.2 (x86-64-v2) is supported by x86-64 CPUs since about 2009. Unlike SSE2, it can vectorize 32-bit integer
# multiplication, which integer image convolution needs. A newer instruction set, e.g. x86-64-v3 or native, runs
# faster on CPUs that have it
set(BOOFCPP_MARCH "x86-64-v2" CACHE STRING "Instruction set of x86-64 builds, passed to -march")
if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64" AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=${BOOFCPP_MARCH}")
endif()

# A shared library must contain everything it needs, so a missing object library fails the build instead of loading
# the JNI library
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-undefined")
endif()

# Remove any ambiguity about what mode it is in
IF(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message("-- Debug Mode --")
ELSE()
    message("-- Release Mode --")
ENDIF()


# Options. Turn on with 'cmake -Dmyvarname=ON'.
option(test "Build all tests." ON) # Makes boolean 'test' available.
enable_testing()

add_subdirectory(libs/common/cpp)
add_subdirectory(libs/strict/cpp)
add_subdirectory(libs/fast/cpp)
add_subdirectory(benchmark)
add_subdirectory(libs/common/jni)
add_subdirectory(libs/strict/jni)
add_subdirectory(libs/fast/jni)

# Every variant's libraries share the thread pool in BoofCPPCommon instead of having their own
if(test AND CMAKE_NM AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_test(NAME shared_thread_pool COMMAND ${CMAKE_COMMAND} -DNM=${CMAKE_NM}
            "-DLIBRARIES=$<TARGET_FILE:BoofCPP>;$<TARGET_FILE:BoofCPPFast>"
            "-DJNI_LIBRARIES=$<TARGET_FILE:JNIBoofCPP>;$<TARGET_FILE:JNIBoofCPPFast>"
            -P ${CMAKE_CURRENT_SOURCE_DIR}/libs/common/cpp/test/check_shared_pool.cmake)
endif()

# Only googletest is needed, not googlemock, and it shouldn't be installed with BoofCPP
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
add_subdirectory(gtest)


