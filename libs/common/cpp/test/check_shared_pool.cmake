# Fails if a library defines BoofConcurrency's functions instead of importing them from BoofCPPCommon, which would give
# it its own thread pool. JNI libraries must also import set_max_threads(), which NativeConcurrency calls.
# Usage: cmake -DNM=<nm> -DLIBRARIES=<a;b> -DJNI_LIBRARIES=<c;d> -P check_shared_pool.cmake
cmake_minimum_required(VERSION 3.16)
foreach(library ${LIBRARIES} ${JNI_LIBRARIES})
    execute_process(COMMAND ${NM} -DC ${library} OUTPUT_VARIABLE symbols RESULT_VARIABLE result)
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "${NM} failed on ${library}")
    endif()
    if(symbols MATCHES "[0-9a-f]+ T boofcv::BoofConcurrency::")
        message(FATAL_ERROR "${library} defines BoofConcurrency instead of linking BoofCPPCommon")
    endif()
    if(library IN_LIST JNI_LIBRARIES AND NOT symbols MATCHES "U boofcv::BoofConcurrency::set_max_threads")
        message(FATAL_ERROR "${library} doesn't import BoofConcurrency::set_max_threads() from BoofCPPCommon")
    endif()
endforeach()
