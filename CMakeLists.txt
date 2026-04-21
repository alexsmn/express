cmake_minimum_required(VERSION 3.16)

file(GLOB express_sources CONFIGURE_DEPENDS
  "express/*.cpp"
  "express/*.h"
)

add_library(express ${express_sources})

target_include_directories(express PUBLIC ".")

target_compile_definitions(express PRIVATE -DEXPRESS_IMPLEMENTATION)

target_compile_features(express PUBLIC cxx_std_17)

if(MSVC)
  target_compile_options(express PRIVATE
    /permissive-
    /W4
    /wd4100 # unreferenced formal parameter
    /wd4267 # conversion from 'size_t' to 'int', possible loss of data
  )
endif(MSVC)

enable_testing()
add_subdirectory(tests)
add_subdirectory(benchmarks)
