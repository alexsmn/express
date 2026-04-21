# Expression parsing library

## Features

* Higly customizable
* Low overhead
* Arena allocators

## Usage

```c++
const char formula[] = "If(2 - 1 - 1, 4 + 2, 3 * 3)";
expression::Expression expression;
expression.Parse(formula);
expression::Value value = expression.Calculate();
```

## Dependencies

* C++17
* GTest (optional for UTs)
* Google Benchmark (optional for benchmarks)

## Benchmarks

Benchmarks are built only when Google Benchmark is available to CMake.

If you are using `vcpkg`, install the package for the same triplet as your
build, for example:

```bat
vcpkg install benchmark:x86-windows
```

Then configure/build the project as usual and build the benchmark target:

```bat
cmake --build <build-dir> --target express_benchmark
```

Run the benchmark executable directly:

```bat
<build-dir>\benchmarks\express_benchmark.exe
```

The benchmark suite covers parse, evaluate, format, traverse, and parse +
evaluate workloads for representative expressions including arithmetic,
function-heavy, variable-heavy, and long-string cases.

### Sample Benchmark Report

The following results were collected on April 21, 2026 from a local debug
build. They are useful as a relative baseline only; absolute timings will be
lower in an optimized release build.

| Workload | Parse CPU | Evaluate CPU | Format CPU | Traverse CPU | Parse + Evaluate CPU |
| --- | ---: | ---: | ---: | ---: | ---: |
| Simple arithmetic | 7.8 us | 625 ns | 6.3 us | 312 ns | 7.8 us |
| Nested precedence | 4.7 us | 469 ns | 6.3 us | 349 ns | 5.4 us |
| Function-heavy | 26.2 us | 312 ns | 17.4 us | 698 ns | 34.9 us |
| Variable-heavy | 14.6 us | 436 ns | 6.3 us | 312 ns | 12.5 us |
| Long string | 17.4 us | 4.7 us | 3.1 us | 312 ns | 7.8 us |

Observed patterns:

* Parsing dominates total cost for every workload in this debug run.
* Pre-parsed evaluation is sub-microsecond for numeric expressions and about
  4.7 microseconds for the string concatenation case.
* Function-heavy expressions are the slowest to parse and to format.
