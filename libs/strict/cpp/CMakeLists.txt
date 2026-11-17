project(boofcpp)

# Strict: results are identical to BoofCV. Namespace boofcv
file(GLOB BOOFCPP_HDR "include/boofcv/*.h")
file(GLOB BOOFCPP_SRC "src/*.cpp")

# Compiled once, then linked into the shared library for C++ users and into the JNI library, so the JNI library needs
# no other BoofCPP library at run time. CMake only links the objects of an OBJECT library into targets that link it
# directly, so those targets also link BoofCPPCommon
add_library(BoofCPPObjects OBJECT ${BOOFCPP_HDR} ${BOOFCPP_SRC})
target_include_directories(BoofCPPObjects PUBLIC include)
target_link_libraries(BoofCPPObjects PUBLIC BoofCPPCommon)
set_target_properties(BoofCPPObjects PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(BoofCPP SHARED)
target_link_libraries(BoofCPP PUBLIC BoofCPPObjects BoofCPPCommon)

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