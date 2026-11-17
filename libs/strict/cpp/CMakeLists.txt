project(boofcpp)

# Strict: results are identical to BoofCV. Namespace boofcv
file(GLOB BOOFCPP_HDR "include/boofcv/*.h")
file(GLOB BOOFCPP_SRC "src/*.cpp")

add_library(BoofCPP SHARED ${BOOFCPP_HDR} ${BOOFCPP_SRC})
target_include_directories(BoofCPP PUBLIC include)
target_link_libraries(BoofCPP PUBLIC BoofCPPCommon)

# Install library
install(TARGETS BoofCPP DESTINATION lib ARCHIVE DESTINATION lib)

if (test)
    include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

    ##############
    # Unit Tests
    ##############
#    file(GLOB TestList "test/*.cpp")
        # then go through the list and remove test/ and .cpp

    list(APPEND TestList test_average_down_sample)
    list(APPEND TestList test_binary_ops)
    list(APPEND TestList test_census_transform)
    list(APPEND TestList test_contour)
    list(APPEND TestList test_convolve)
    list(APPEND TestList test_convolve_kernels)
    list(APPEND TestList test_disparity_block_match)
    list(APPEND TestList test_image_blur)
    list(APPEND TestList test_image_convert)
    list(APPEND TestList test_image_normalization)
    list(APPEND TestList test_image_statistics)
    list(APPEND TestList test_pixel_math)
    list(APPEND TestList test_stereo_disparity)
    list(APPEND TestList test_threshold_block_filters)

    FOREACH(FILENAME ${TestList})
        add_executable(${FILENAME} test/${FILENAME}.cpp)
        target_link_libraries(${FILENAME} gtest gtest_main BoofCPP BoofCPPCommonTest)
        add_test(${FILENAME} ${FILENAME})
    ENDFOREACH()
endif()