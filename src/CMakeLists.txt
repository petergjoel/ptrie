cmake_minimum_required(VERSION 3.7)
project(PTrie C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_INCLUDE_CURRENT_DIR ON)


add_library(ptrie binarywrapper.h binarywrapper.cpp ${HEADER_FILES})
target_include_directories (ptrie PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

install(TARGETS ptrie
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)
install (FILES  binarywrapper.h linked_bucket.h ptrie.h ptrie_map.h ptrie_stable.h DESTINATION include/ptrie)

