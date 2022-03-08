cmake_minimum_required(VERSION 3.7)
include(CMakePackageConfigHelpers)
project(PTrie C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_library(ptrie INTERFACE ${HEADER_FILES})

install(TARGETS ptrie
        EXPORT ptrie
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)
install (FILES  ptrie/linked_bucket.h ptrie/ptrie.h ptrie/ptrie_map.h ptrie/ptrie_stable.h DESTINATION include/ptrie)

set(INCLUDE_INSTALL_DIR include/)
set(LIB_INSTALL_DIR lib/)

configure_package_config_file(ptrieConfig.cmake.in
	${CMAKE_CURRENT_BINARY_DIR}/ptrieConfig.cmake
	INSTALL_DESTINATION ${LIB_INSTALL_DIR}/ptrie/cmake
	PATH_VARS INCLUDE_INSTALL_DIR)
write_basic_package_version_file(
	${CMAKE_CURRENT_BINARY_DIR}/ptrieConfigVersion.cmake
	VERSION 1.1.1
	COMPATIBILITY SameMajorVersion )
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/ptrieConfig.cmake
	${CMAKE_CURRENT_BINARY_DIR}/ptrieConfigVersion.cmake
	DESTINATION ${LIB_INSTALL_DIR}/ptrie/cmake )

install(EXPORT ptrie FILE ptrie.cmake DESTINATION ${LIB_INSTALL_DIR}/ptrie/cmake)
