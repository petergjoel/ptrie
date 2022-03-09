cmake_minimum_required(VERSION 3.7)

cmake_policy(SET CMP0048 NEW)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(PTRIE_VERSION 1.1.1)
project(ptrie VERSION ${PTRIE_VERSION} LANGUAGES CXX)

# MAIN_PROJECT CHECK
# Determine if ptrie is built as a subproject (using add_subdirectory) or if it is the main project
set(PTRIE_MAIN_PROJECT OFF)
if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(PTRIE_MAIN_PROJECT ON)
endif()

option(PTRIE_BuildTests "Build the unit tests when BUILD_TESTING is enabled." ${PTRIE_MAIN_PROJECT})
option(PTRIE_BuildBenchmark "Build the simple benchmark suite" OFF)
option(PTRIE_AddressSanitizer "Enables address sanitization during compilation." OFF)
option(PTRIE_GetDependencies "Fetch external dependencies from web." OFF)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif(NOT CMAKE_BUILD_TYPE)

if(PTRIE_AddressSanitizer)
    add_compile_options(-fno-omit-frame-pointer -fsanitize=address)
endif(PTRIE_AddressSanitizer)


if (PTRIE_GetDependencies)
    # setup for external imports
    include(ExternalProject)
    set(EXTERNAL_INSTALL_LOCATION ${CMAKE_BINARY_DIR}/external)

    file(MAKE_DIRECTORY ${EXTERNAL_INSTALL_LOCATION}/include)
    file(MAKE_DIRECTORY ${EXTERNAL_INSTALL_LOCATION}/lib)
    if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        set(BOOST_CONFIG_JAM "using clang : : ${CMAKE_CXX_COMPILER}")
        set(BOOST_B2_OPTS "")
        set(BOOST_TOOLSET "clang")
    elseif (UNIX AND NOT APPLE)
        set(BOOST_CONFIG_JAM "using gcc : : ${CMAKE_CXX_COMPILER}")
        set(BOOST_B2_OPTS "")
        set(BOOST_TOOLSET "gcc")
    elseif (APPLE)
        set(BOOST_CONFIG_JAM "using clang : : ${CMAKE_CXX_COMPILER}")
        set(BOOST_B2_OPTS "")
        set(BOOST_TOOLSET "clang")
    else ()
        set(BOOST_CONFIG_JAM "using gcc : : ${CMAKE_CXX_COMPILER}")
        set(BOOST_B2_OPTS target-os=windows binary-format=pe abi=ms threading=multi threadapi=win32)
        set(BOOST_TOOLSET "gcc")
    endif ()

    if (ARCH_TYPE MATCHES "win64")
        set(BOOST_CONFIG_JAM "using gcc : : ${CMAKE_CXX_COMPILER}")
    endif ()

    if(BUILD_TESTING AND PTRIE_BuildTests)
        ExternalProject_Add(
            boost-ext
            URL https://boostorg.jfrog.io/artifactory/main/release/1.78.0/source/boost_1_78_0.zip
            URL_HASH SHA256=f22143b5528e081123c3c5ed437e92f648fe69748e95fa6e2bd41484e2986cc3
            PATCH_COMMAND echo "${BOOST_CONFIG_JAM}" $<SEMICOLON>  > config.jam
            CONFIGURE_COMMAND ./bootstrap.sh --with-toolset=${BOOST_TOOLSET} --prefix=${CMAKE_BINARY_DIR}/external cxxflags="-arch x86_64" --without-icu
            BUILD_COMMAND ./b2 --user-config=config.jam --prefix=${CMAKE_BINARY_DIR}/external --with-test cxxstd=17 cflags="-fPIC" cxxflags="-fPIC" address-model=64 ${BOOST_B2_OPTS} variant=release link=shared runtime-link=shared install
            BUILD_IN_SOURCE 1
            INSTALL_COMMAND ""
        )
    endif()

    # we can now include external libraries
    set(CMAKE_PREFIX_PATH ${EXTERNAL_INSTALL_LOCATION};${CMAKE_PREFIX_PATH})
    include_directories(${EXTERNAL_INSTALL_LOCATION}/include)
    link_directories(${EXTERNAL_INSTALL_LOCATION}/lib)
endif (PTRIE_GetDependencies)

if(NOT CMAKE_BUILD_TYPE MATCHES Prebuild)
    add_subdirectory(src)
    if(PTRIE_BuildBenchmark)
        #benchmark
        add_subdirectory(benchmark)
    endif()
    if(BUILD_TESTING AND PTRIE_BuildTests)
        #testing
        enable_testing()
        add_subdirectory(test)
    endif()
else()
    add_custom_target( prebuild )
    add_dependencies(prebuild boost-ext)
endif()
