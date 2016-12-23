

find_package (Boost COMPONENTS unit_test_framework REQUIRED)
include_directories (${TEST_SOURCE_DIR}/src
                     ${Boost_INCLUDE_DIRS}
                     ${PTrie_SOURCE_DIR}
                     )
add_definitions (-DBOOST_TEST_DYN_LINK)

add_executable (Wrapper wrapper.cpp)
add_executable(Delete delete.cpp)
add_executable (Set set.cpp)
add_executable (Map map.cpp)
add_executable (StableSet stable_set.cpp)


target_link_libraries(Wrapper   boost_unit_test_framework ptrie)
target_link_libraries(Set       boost_unit_test_framework ptrie)
target_link_libraries(Delete    boost_unit_test_framework ptrie)
target_link_libraries(StableSet boost_unit_test_framework ptrie)
target_link_libraries(Map       boost_unit_test_framework ptrie)