project(boofcpp_fast)

# Fast: results are within floating point tolerance of BoofCV. Namespace boofcv::fast
file(GLOB BOOFCPP_FAST_HDR "include/boofcv/fast/*.h")
file(GLOB BOOFCPP_FAST_SRC "src/*.cpp")

add_library(BoofCPPFast SHARED ${BOOFCPP_FAST_HDR} ${BOOFCPP_FAST_SRC})
target_include_directories(BoofCPPFast PUBLIC include)
target_link_libraries(BoofCPPFast PUBLIC BoofCPPCommon)

install(TARGETS BoofCPPFast DESTINATION lib ARCHIVE DESTINATION lib)

if (test)
    include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

    # The shared checks compare against strict's naive implementations, so the tests link strict
    list(APPEND FastTestList test_image_blur)

    FOREACH(FILENAME ${FastTestList})
        add_executable(fast_${FILENAME} test/${FILENAME}.cpp)
        target_include_directories(fast_${FILENAME} PRIVATE test)
        target_link_libraries(fast_${FILENAME} gtest gtest_main BoofCPPFast BoofCPP BoofCPPCommonTest BoofCPPStrictTest)
        add_test(fast_${FILENAME} fast_${FILENAME})
    ENDFOREACH()
endif()
