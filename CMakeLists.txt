cmake_minimum_required(VERSION 2.8)
project(ptrie C CXX)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif(NOT CMAKE_BUILD_TYPE)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O2 -DNDEBUG -Wall")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2 -DNDEBUG -Wall")
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
add_test(NAME Delete    COMMAND Delete)
add_test(NAME StableSet COMMAND StableSet)
add_test(NAME Map       COMMAND Map)
