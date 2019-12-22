if(NOT GTEST_FOUND)
	find_package(GTest)
endif()

if(GTEST_FOUND)
  add_executable(express_unittest "test.cpp")
  target_link_libraries(express_unittest PUBLIC express gtest_main)

  include(GoogleTest)
  gtest_discover_tests(express_unittest)
endif()
