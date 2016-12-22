cmake_minimum_required(VERSION 2.8)
project(PTrie C CXX)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_INCLUDE_CURRENT_DIR ON)


add_library(ptrie binarywrapper.h binarywrapper.cpp ${HEADER_FILES})
target_include_directories (ptrie PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})