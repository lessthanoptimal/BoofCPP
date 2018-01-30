cmake_minimum_required(VERSION 3.2.2)
project(boofcpp_all)

# Options. Turn on with 'cmake -Dmyvarname=ON'.
option(test "Build all tests." ON) # Makes boolean 'test' available.
enable_testing()

add_subdirectory(boofcpp)
add_subdirectory(jni)

add_subdirectory(gtest)


