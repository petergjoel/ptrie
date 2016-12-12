cmake_minimum_required(VERSION 3.6)
project(ptrie C CXX)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif(NOT CMAKE_BUILD_TYPE)

set(CMAKE_CXX_STANDARD 14)

#actual library
add_subdirectory(src)

#benchmark
add_subdirectory(benchmark)

#testing
add_subdirectory(test)
enable_testing()
add_test(NAME Wrapper   COMMAND Wrapper)
add_test(NAME Set       COMMAND Set)
add_test(NAME StableSet COMMAND StableSet)
add_test(NAME Map       COMMAND Map)