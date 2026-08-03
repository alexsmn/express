find_package(benchmark CONFIG QUIET)

if(TARGET benchmark::benchmark_main)
  add_executable(express_benchmark "benchmark.cpp")
  target_link_libraries(express_benchmark
    PUBLIC
      express
      benchmark::benchmark_main
  )
endif()
