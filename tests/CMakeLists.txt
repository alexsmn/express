# GTest stays OPTIONAL here — express's CLAUDE.md makes being dependency-free
# the library's policy, and vcpkg.json carries gtest as the `tests` feature
# rather than a dependency.
#
# What was wrong is the gate, not the policy. It tested `GTEST_FOUND`, which
# only CMake's module-mode FindGTest sets; vcpkg ships GTest as a CONFIG
# package, which sets `GTest_FOUND`. So the condition has never been true in
# this tree and the five *_test.cpp files below have never been compiled, in any
# build, standalone or composed. Gate on the imported target instead: it exists
# whichever mode found the package, and it is what the executable links.
find_package(GTest CONFIG QUIET)

if(TARGET GTest::gtest_main)
  file(GLOB express_test_sources CONFIGURE_DEPENDS
    "../express/*_test.cpp"
  )

  add_executable(express_unittest ${express_test_sources})
  target_link_libraries(express_unittest PUBLIC express GTest::gtest_main)

  include(GoogleTest)
  gtest_discover_tests(express_unittest)
else()
  message(STATUS
    "express: GTest not found, skipping express_unittest "
    "(configure with the `tests` vcpkg feature to build it)")
endif()
