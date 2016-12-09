

find_package (Boost COMPONENTS unit_test_framework REQUIRED)
include_directories (${TEST_SOURCE_DIR}/src
                     ${Boost_INCLUDE_DIRS}
                     ${PTrie_SOURCE_DIR}
                     )
add_definitions (-DBOOST_TEST_DYN_LINK)

add_executable (Wrapper wrapper.cpp)
add_executable (Insertion insertion.cpp)
add_executable (Deletion deletion.cpp)
add_executable (Existence existence.cpp)


target_link_libraries(Wrapper   boost_unit_test_framework ptrie)
target_link_libraries(Insertion boost_unit_test_framework ptrie)
target_link_libraries(Existence boost_unit_test_framework ptrie)
target_link_libraries(Deletion  boost_unit_test_framework ptrie)