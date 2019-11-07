find_package(GTest REQUIRED)

add_executable(express_unittest "test.cpp")
target_link_libraries(express_unittest PUBLIC express GTest::Main)

include(GoogleTest)
gtest_discover_tests(express_unittest)
