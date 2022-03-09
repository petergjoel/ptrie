
find_package (Boost 1.70 REQUIRED COMPONENTS unit_test_framework)
add_definitions (-DBOOST_TEST_DYN_LINK)

if (MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

add_executable (Delete delete.cpp)
add_executable (Set set.cpp)
add_executable (Map map.cpp)
add_executable (StableSet stable_set.cpp)

target_link_libraries(Set       PRIVATE Boost::unit_test_framework ptrie)
target_link_libraries(Delete    PRIVATE Boost::unit_test_framework ptrie)
target_link_libraries(StableSet PRIVATE Boost::unit_test_framework ptrie)
target_link_libraries(Map       PRIVATE Boost::unit_test_framework ptrie)

add_test(NAME Set       COMMAND Set)
add_test(NAME Delete    COMMAND Delete)
add_test(NAME StableSet COMMAND StableSet)
add_test(NAME Map       COMMAND Map)
