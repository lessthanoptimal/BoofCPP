project(boofcpp_common)

# Image types, borders, configurations and other code that every variant (strict, fast) shares. Namespace boofcv.
# A shared library that every variant's libraries link, so variants used together share one BoofConcurrency thread
# pool and its settings
file(GLOB COMMON_HDR "include/boofcv/*.h")
file(GLOB COMMON_SRC "src/*.cpp")

add_library(BoofCPPCommon SHARED ${COMMON_HDR} ${COMMON_SRC})
target_include_directories(BoofCPPCommon PUBLIC include)
# BoofConcurrency's thread pool
find_package(Threads REQUIRED)
target_link_libraries(BoofCPPCommon PUBLIC Threads::Threads)
# Each variant's native jar packages it from here
set_target_properties(BoofCPPCommon PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/boofcpp-common
        WINDOWS_EXPORT_ALL_SYMBOLS ON)

install(TARGETS BoofCPPCommon DESTINATION lib ARCHIVE DESTINATION lib)

if (test)
    # Helpers and checks shared by the unit tests of every variant, e.g. testing_utils.h and shared/
    add_library(BoofCPPCommonTest INTERFACE)
    target_include_directories(BoofCPPCommonTest INTERFACE test)

    include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

    list(APPEND CommonTestList test_base_types)
    list(APPEND CommonTestList test_concurrency)
    list(APPEND CommonTestList test_config_disparity)
    list(APPEND CommonTestList test_config_types)
    list(APPEND CommonTestList test_convolve_kernels)
    list(APPEND CommonTestList test_convolve_naive)
    list(APPEND CommonTestList test_geometry_types)
    list(APPEND CommonTestList test_image_border)
    list(APPEND CommonTestList test_image_misc_ops)
    list(APPEND CommonTestList test_image_types)
    list(APPEND CommonTestList test_mean_divisor)
    list(APPEND CommonTestList test_packed_sets)
    list(APPEND CommonTestList test_sanity_checks)
    list(APPEND CommonTestList test_testing_utils)
    list(APPEND CommonTestList test_code_standards)

    FOREACH(FILENAME ${CommonTestList})
        add_executable(${FILENAME} test/${FILENAME}.cpp)
        target_link_libraries(${FILENAME} gtest gtest_main BoofCPPCommon BoofCPPCommonTest)
        add_test(${FILENAME} ${FILENAME})
    ENDFOREACH()
    # test_code_standards reads the tests of every library
    target_compile_definitions(test_code_standards PRIVATE BOOFCPP_LIBS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../..")
endif()
