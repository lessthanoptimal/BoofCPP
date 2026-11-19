project(boofcpp_fast)

# Fast: results are within floating point tolerance of BoofCV. Namespace boofcv::fast
file(GLOB BOOFCPP_FAST_HDR "include/boofcv/fast/*.h")
file(GLOB BOOFCPP_FAST_SRC "src/*.cpp")

# Linked into the shared library and the JNI library, the same as strict's BoofCPPObjects. Fast uses strict's code for
# what it doesn't speed up, so targets that link it also link strict's objects
add_library(BoofCPPFastObjects OBJECT ${BOOFCPP_FAST_HDR} ${BOOFCPP_FAST_SRC})
target_include_directories(BoofCPPFastObjects PUBLIC include)
target_link_libraries(BoofCPPFastObjects PUBLIC BoofCPPObjects BoofCPPCommon)
set_target_properties(BoofCPPFastObjects PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(BoofCPPFast SHARED)
target_link_libraries(BoofCPPFast PUBLIC BoofCPPFastObjects BoofCPPObjects BoofCPPCommon)

install(TARGETS BoofCPPFast DESTINATION lib ARCHIVE DESTINATION lib)

if (test)
    include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

    list(APPEND FastTestList test_image_blur)

    FOREACH(FILENAME ${FastTestList})
        add_executable(fast_${FILENAME} test/${FILENAME}.cpp)
        target_include_directories(fast_${FILENAME} PRIVATE test)
        target_link_libraries(fast_${FILENAME} gtest gtest_main BoofCPPFastObjects BoofCPPObjects BoofCPPCommon
                BoofCPPCommonTest)
        add_test(fast_${FILENAME} fast_${FILENAME})
    ENDFOREACH()
endif()
