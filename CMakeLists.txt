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


