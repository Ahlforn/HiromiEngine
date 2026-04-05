find_package(GTest CONFIG REQUIRED)

add_executable(hiromi_tests
    test_ecs.cpp
    test_query.cpp
)

target_link_libraries(hiromi_tests
    PRIVATE
        HiromiEngine
        GTest::gtest
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(hiromi_tests)
