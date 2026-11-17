#ifndef BOOFCPP_BENCHMARK_COMMON_H
#define BOOFCPP_BENCHMARK_COMMON_H

#include "boofcv/image_types.h"
#include "boofcv/base_types.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <functional>
#include <vector>

namespace boofcv {
    bool load_jpeg( const char *filename , Interleaved<U8>& dst );
    bool load_png( const char *filename ,  Interleaved<U8>& dst );
}

/// Runs the function N times and prints the mean and minimum time in milliseconds
inline void bench( const char* name, int N, const std::function<void()>& f ) {
    f(); // warm up
    std::vector<double> times;
    for( int i = 0; i < N; i++ ) {
        auto t0 = std::chrono::steady_clock::now();
        f();
        auto t1 = std::chrono::steady_clock::now();
        times.push_back(std::chrono::duration<double,std::milli>(t1-t0).count());
    }
    double sum = 0; for( double t : times ) sum += t;
    printf("%28s mean = %9.4f min = %9.4f (ms)\n", name, sum/N, *std::min_element(times.begin(),times.end()));
}

#endif
