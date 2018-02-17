project(benchmark)

if(UNIX AND NOT APPLE)
    set(LINUX TRUE)
endif()

set(CMAKE_CXX_STANDARD 11)

FIND_PACKAGE(JPEG)
IF(JPEG_FOUND)
    INCLUDE_DIRECTORIES(${JPEG_INCLUDE_DIRS})
ENDIF()

FIND_PACKAGE(PNG)
IF(PNG_FOUND)
    INCLUDE_DIRECTORIES(${PNG_INCLUDE_DIRS})
ENDIF()

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

include_directories(${CMAKE_SOURCE_DIR}/boofcpp/src)

#add_library(benchmark_common SHARED ${BENCHMARK_COMMON} ${IMAGE_LIBS})

add_executable(thresholding src/thresholding.cpp ${BENCHMARK_COMMON})

list(APPEND BechmarkList thresholding)


FOREACH(FILENAME ${BechmarkList})
    target_link_libraries(${FILENAME} BoofCPP ${IMAGE_LIBS})
ENDFOREACH()
