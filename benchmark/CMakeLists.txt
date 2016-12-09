project(benchmark)

include_directories(${libptrie_SOURCE_DIR})

add_executable(benchmark benchmark.cpp)
target_link_libraries(benchmark LINK_PUBLIC ptrie)

