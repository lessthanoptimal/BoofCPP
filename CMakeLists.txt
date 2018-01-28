cmake_minimum_required(VERSION 3.9)
project(BoofCPP)

set(CMAKE_CXX_STANDARD 11)

file(GLOB BOOFCPP_HDR "src/boofcv/*.h")
file(GLOB BOOFCPP_SRC "src/boofcv/*.cpp")

add_library(BoofCPP SHARED ${BOOFCPP_HDR} ${BOOFCPP_SRC})

add_subdirectory(gtest)

if (test)
    include_directories(${BOOFCPP_HDR})
    include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

    ##############
    # Unit Tests
    ##############
    add_executable(test_sanity_checks test/test_sanity_checks.cpp)
    target_link_libraries(test_sanity_checks gtest gtest_main BoofCPP)
    add_test(test_identify test_identify)
endif()