cmake_minimum_required(VERSION 3.9)
project(BoofCPP)

set(CMAKE_CXX_STANDARD 11)

file(GLOB BOOFCPP_SRC "src/boofcv/*.h" "src/boofcv/*.cpp")

add_library(BoofCPP SHARED ${BOOFCPP_SRC})

add_subdirectory(gtest)

if (test)
    include_directories(${CMAKE_SOURCE_DIR}/detector/src/main/c)
    include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

    ##############
    # Unit Tests
    ##############
    add_executable(test_identify src/test/c/test_identify.cpp)
    target_link_libraries(test_identify gtest gtest_main quirc)
    add_test(test_identify test_identify)
endif()