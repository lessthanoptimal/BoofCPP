project(benchmark)

if(UNIX AND NOT APPLE)
    set(LINUX TRUE)
endif()

set(CMAKE_CXX_STANDARD 11)

if (LINUX)
    set(IMAGE_LIBS m jpeg png)
elseif(APPLE)
    set(IMAGE_LIBS libjpeg.a libpng.a)
endif()


file(GLOB BENCHMARK_COMMON "src/benchmark_*.*")
file(GLOB BENCHMARK_HEADERS "src/benchmark_*.h")

include_directories(${CMAKE_SOURCE_DIR}/boofcpp/src)

#add_library(benchmark_common SHARED ${BENCHMARK_COMMON} ${IMAGE_LIBS})

add_executable(thresholding src/thresholding.cpp ${BENCHMARK_COMMON})

list(APPEND BechmarkList thresholding)


FOREACH(FILENAME ${BechmarkList})
    target_link_libraries(${FILENAME} BoofCPP ${IMAGE_LIBS})
ENDFOREACH()
