project(benchmark)

include_directories(${libptrie_SOURCE_DIR})
add_library(murmur MurmurHash2.h MurmurHash2.cpp)
add_executable(benchmark benchmark.cpp utils.h binarywrapper.cpp)
add_executable(int_benchmark int_benchmark.cpp utils.h binarywrapper.cpp)

target_link_libraries(int_benchmark LINK_PUBLIC ptrie murmur)
target_link_libraries(benchmark LINK_PUBLIC ptrie murmur)

