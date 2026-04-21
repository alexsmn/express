if(NOT GTEST_FOUND)
  find_package(GTest)
endif()

if(GTEST_FOUND)
  file(GLOB express_test_sources CONFIGURE_DEPENDS
    "../express/*_test.cpp"
  )

  add_executable(express_unittest ${express_test_sources})
  target_link_libraries(express_unittest PUBLIC express GTest::gtest_main)

  include(GoogleTest)
  gtest_discover_tests(express_unittest)
endif()
