file(GLOB express_sources
  "express/*.cpp"
  "express/*.h"
)

add_library(express ${express_sources})

target_include_directories(express PUBLIC ".")

target_compile_definitions(express PRIVATE -DEXPRESS_IMPLEMENTATION)

target_compile_features(express PUBLIC cxx_std_17)

if(WIN32)
  target_compile_options(express PRIVATE "/Za" "/permissive-")
endif()

enable_testing()

add_subdirectory(tests)