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
#    file(GLOB TestList "test/*.cpp")
        # then go through the list and remove test/ and .cpp

    list(APPEND TestList test_base_types)
    list(APPEND TestList test_binary_ops)
    list(APPEND TestList test_config_types)
    list(APPEND TestList test_convolve)
    list(APPEND TestList test_geometry_types)
    list(APPEND TestList test_image_blur)
    list(APPEND TestList test_image_statistics)
    list(APPEND TestList test_image_misc_ops)
    list(APPEND TestList test_image_types)
    list(APPEND TestList test_sanity_checks)
    list(APPEND TestList test_threshold_block_filters)

    FOREACH(FILENAME ${TestList})
        add_executable(${FILENAME} test/${FILENAME}.cpp)
        target_link_libraries(${FILENAME} gtest gtest_main BoofCPP)
        add_test(${FILENAME} ${FILENAME})
    ENDFOREACH()
endif()