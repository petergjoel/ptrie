cmake_minimum_required(VERSION 3.7)
project(PTrie C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_library(ptrie INTERFACE ${HEADER_FILES})
target_include_directories (ptrie INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

install(TARGETS ptrie
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)
install (FILES  ptrie/linked_bucket.h ptrie/ptrie.h ptrie/ptrie_map.h ptrie/ptrie_stable.h DESTINATION include/ptrie)

