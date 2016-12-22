project(benchmark)

include_directories(${libptrie_SOURCE_DIR})
add_library(murmur MurmurHash2.h MurmurHash2.cpp)
add_executable(benchmark benchmark.cpp)
target_link_libraries(benchmark LINK_PUBLIC ptrie murmur tbb jemalloc)

