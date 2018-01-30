cmake_minimum_required(VERSION 3.9)
project(boofcpp)

set(CMAKE_CXX_STANDARD 11)

file(GLOB BOOFCPP_HDR "src/boofcv/*.h")
file(GLOB BOOFCPP_SRC "src/boofcv/*.cpp")

add_library(BoofCPP SHARED ${BOOFCPP_HDR} ${BOOFCPP_SRC})

if (test)
    include_directories("src/boofcv/")
    include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

    ##############
    # Unit Tests
    ##############
    add_executable(test_base_types test/test_base_types.cpp)
    add_executable(test_binary_ops test/test_binary_ops.cpp)
    add_executable(test_config_types test/test_config_types.cpp)
    add_executable(test_image_types test/test_image_types.cpp)
    add_executable(test_sanity_checks test/test_sanity_checks.cpp)
    target_link_libraries(test_base_types gtest gtest_main BoofCPP)
    target_link_libraries(test_binary_ops gtest gtest_main BoofCPP)
    target_link_libraries(test_config_types gtest gtest_main BoofCPP)
    target_link_libraries(test_image_types gtest gtest_main BoofCPP)
    target_link_libraries(test_sanity_checks gtest gtest_main BoofCPP)
    add_test(test_base_types test_base_types)
    add_test(test_binary_ops test_binary_ops)
    add_test(test_config_types test_config_types)
    add_test(test_image_types test_image_types)
    add_test(test_sanity_checks test_sanity_checks)
endif()