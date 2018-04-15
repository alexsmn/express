file(GLOB express_sources
  "express/*.cpp"
  "express/*.h"
)

add_library(express ${express_sources})

target_include_directories(express PUBLIC ".")

target_compile_definitions(express PRIVATE -DEXPRESS_IMPLEMENTATION)

# Conformance mode
if(WIN32)
  target_compile_options(express PRIVATE "/Za" "/permissive-")
endif()

target_link_libraries(express base)

add_subdirectory(tests)