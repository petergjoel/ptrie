project(benchmark)

include_directories(${libptrie_SOURCE_DIR})
add_library(murmur MurmurHash2.h MurmurHash2.cpp)
add_executable(benchmark benchmark.cpp utils.h)
add_executable(int_benchmark int_benchmark.cpp utils.h)

target_link_libraries(int_benchmark LINK_PUBLIC ptrie murmur tbb)
target_link_libraries(benchmark LINK_PUBLIC ptrie murmur tbb)

