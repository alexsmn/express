cmake_minimum_required(VERSION 3.16)

# The shared build-support kit (ADR 0011): at this product's root in an export,
# at the tree root in the monorepo. Before project(), so the machine config it
# reads can still set the vcpkg variables the toolchain consumes there.
set(_scada_dir "${CMAKE_CURRENT_SOURCE_DIR}")
while(NOT EXISTS "${_scada_dir}/build-support/ScadaProducts.cmake")
  get_filename_component(_scada_parent "${_scada_dir}" DIRECTORY)
  if(_scada_parent STREQUAL _scada_dir)
    message(FATAL_ERROR
      "build-support/ not found above ${CMAKE_CURRENT_SOURCE_DIR}")
  endif()
  set(_scada_dir "${_scada_parent}")
endwhile()
include("${_scada_dir}/build-support/ScadaProducts.cmake")
include("${_scada_dir}/build-support/ScadaProductBase.cmake")
unset(_scada_dir)
unset(_scada_parent)

scada_product_prologue()

# There was no project() call at all. As a subdirectory of a consumer that did
# not matter; as the top-level file CMake warns and invents one, and every
# PROJECT_* variable the kit reads is then someone else's.
project(express VERSION 1.0.0 LANGUAGES CXX)

scada_product_base()

file(GLOB express_sources CONFIGURE_DEPENDS
  "express/*.cpp"
  "express/*.h"
)
list(FILTER express_sources EXCLUDE REGEX ".*_test\\.cpp$")

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
