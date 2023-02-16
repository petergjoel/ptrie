cmake_minimum_required(VERSION 3.7)
include(CMakePackageConfigHelpers)
project(PTrie VERSION ${PTRIE_VERSION} LANGUAGES CXX)

include(GNUInstallDirs) # With GNUInstallDirs we use platform-independent macros to get the correct install directory names.  (CMAKE_INSTALL_BINDIR, CMAKE_INSTALL_LIBDIR, CMAKE_INSTALL_INCLUDEDIR)

add_library(ptrie INTERFACE ${HEADER_FILES})
target_compile_features(ptrie INTERFACE cxx_std_20) # Require C++20 features.
find_package (Boost 1.70 REQUIRED COMPONENTS headers)
target_link_libraries(ptrie INTERFACE Boost::headers)
target_include_directories(ptrie INTERFACE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
	$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
) # The correct include directory depends on whether we are building or installing ptrie.
# Define which directories to install with the ptrie library.
install(DIRECTORY ptrie/
		DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ptrie)

install(TARGETS  ptrie
		EXPORT   ptrie
		RUNTIME  DESTINATION ${CMAKE_INSTALL_BINDIR}
		LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
		ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR})

set(INCLUDE_INSTALL_DIR ${CMAKE_INSTALL_INCLUDEDIR})

configure_package_config_file(ptrieConfig.cmake.in
	${CMAKE_CURRENT_BINARY_DIR}/ptrieConfig.cmake
	INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/ptrie/cmake
	PATH_VARS INCLUDE_INSTALL_DIR)
write_basic_package_version_file(
	${CMAKE_CURRENT_BINARY_DIR}/ptrieConfigVersion.cmake
	VERSION ${PTRIE_VERSION}
	COMPATIBILITY SameMajorVersion )
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/ptrieConfig.cmake
	${CMAKE_CURRENT_BINARY_DIR}/ptrieConfigVersion.cmake
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/ptrie/cmake )

install(EXPORT ptrie FILE ptrie.cmake DESTINATION ${CMAKE_INSTALL_LIBDIR}/ptrie/cmake)
