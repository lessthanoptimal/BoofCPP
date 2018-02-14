project(benchmark)

set(CMAKE_CXX_STANDARD 11)

file(GLOB BENCHMARK_COMMON "src/benchmark_*.*")
file(GLOB BENCHMARK_HEADERS "src/benchmark_*.h")

include_directories(${CMAKE_SOURCE_DIR}/boofcpp/src)

add_library(benchmark_common SHARED ${BENCHMARK_COMMON})

add_executable(thresholding src/thresholding.cpp ${BENCHMARK_HEADERS})

list(APPEND BechmarkList thresholding)

if(UNIX AND NOT APPLE)
    set(LINUX TRUE)
endif()

FOREACH(FILENAME ${BechmarkList})
    if (LINUX)
        target_link_libraries(${FILENAME} BoofCPP benchmark_common m jpeg png)
    elseif(APPLE)
        target_link_libraries(${FILENAME} BoofCPP benchmark_common libjpeg.a libpng.a)
    endif()
ENDFOREACH()
