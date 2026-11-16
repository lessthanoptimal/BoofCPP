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
# multiplication, which integer image convolution needs
if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64" AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=x86-64-v2")
endif()

# Remove any ambiguity about what mode it is in
IF(CMAKE_BUILD_TYPE MATCHES DEBUG)
    message("-- Debug Mode --")
ELSE()
    message("-- Release Mode --")
ENDIF()


# Options. Turn on with 'cmake -Dmyvarname=ON'.
option(test "Build all tests." ON) # Makes boolean 'test' available.
enable_testing()

add_subdirectory(boofcpp)
add_subdirectory(benchmark)
add_subdirectory(boofcpp-jni)

# Only googletest is needed, not googlemock, and it shouldn't be installed with BoofCPP
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
add_subdirectory(gtest)


