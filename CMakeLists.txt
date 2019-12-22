cmake_minimum_required(VERSION 3.16)

file(GLOB express_sources
  "express/*.cpp"
  "express/*.h"
)

add_library(express ${express_sources})

target_include_directories(express PUBLIC ".")

target_compile_definitions(express PRIVATE -DEXPRESS_IMPLEMENTATION)

target_compile_features(express PUBLIC cxx_std_17)

enable_testing()
add_subdirectory(tests)
