project(benchmark)

if(UNIX AND NOT APPLE)
    set(LINUX TRUE)
endif()

# The benchmarks load their test images with libjpeg and libpng. Without them, everything else still builds
FIND_PACKAGE(JPEG)
FIND_PACKAGE(PNG)
IF(NOT JPEG_FOUND OR NOT PNG_FOUND)
    message(STATUS "libjpeg or libpng not found. Skipping the C++ benchmarks")
    return()
ENDIF()
INCLUDE_DIRECTORIES(${JPEG_INCLUDE_DIRS} ${PNG_INCLUDE_DIRS})

set(IMAGE_LIBS ${JPEG_LIBRARIES} ${PNG_LIBRARIES})

# Is this needed?
if (LINUX)
    list(APPEND IMAGE_LIBS m)
#    set(IMAGE_LIBS m ${JPEG_LIBRARIES} ${PNG_LIBRARIES})
#elseif(APPLE)
#    set(IMAGE_LIBS ${JPEG_LIBRARIES} ${PNG_LIBRARIES})
endif()


file(GLOB BENCHMARK_COMMON "src/benchmark_*.*")
file(GLOB BENCHMARK_HEADERS "src/benchmark_*.h")

#add_library(benchmark_common SHARED ${BENCHMARK_COMMON} ${IMAGE_LIBS})

add_executable(thresholding src/thresholding.cpp ${BENCHMARK_COMMON})
list(APPEND BechmarkList thresholding)

add_executable(convolution src/convolution.cpp ${BENCHMARK_COMMON})
list(APPEND BechmarkList convolution)

add_executable(float_ops src/float_ops.cpp ${BENCHMARK_COMMON})
list(APPEND BechmarkList float_ops)

add_executable(disparity src/disparity.cpp ${BENCHMARK_COMMON})
list(APPEND BechmarkList disparity)


FOREACH(FILENAME ${BechmarkList})
    target_link_libraries(${FILENAME} BoofCPP ${IMAGE_LIBS})
ENDFOREACH()

# Fast against strict
add_executable(fast_blur src/fast_blur.cpp ${BENCHMARK_COMMON})
target_link_libraries(fast_blur BoofCPPFast BoofCPP ${IMAGE_LIBS})
