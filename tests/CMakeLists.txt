add_executable(express_unittest "test.cpp")
target_include_directories(express_unittest PUBLIC
	${GMOCK_INCLUDE_DIR}
	${GTEST_INCLUDE_DIR}
)
target_link_libraries(express_unittest PUBLIC express gmock_main)

add_test(express_unittest COMMAND express_unittest)
