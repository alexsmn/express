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

The following results were collected on April 20, 2026 from a local debug
build. They are useful as a relative baseline only; absolute timings will be
lower in an optimized release build.

| Workload | Parse CPU | Evaluate CPU | Format CPU | Traverse CPU | Parse + Evaluate CPU |
| --- | ---: | ---: | ---: | ---: | ---: |
| Simple arithmetic | 26.2 us | 436 ns | 3.1 us | 312 ns | 31.2 us |
| Nested precedence | 19.5 us | 349 ns | 3.1 us | 312 ns | 17.4 us |
| Function-heavy | 61.1 us | 312 ns | 17.4 us | 781 ns | 62.5 us |
| Variable-heavy | 46.9 us | 625 ns | 4.7 us | 312 ns | 46.9 us |
| Long string | 31.2 us | 4.4 us | 2.6 us | 174 ns | 26.2 us |

Observed patterns:

* Parsing dominates total cost for every workload in this debug run.
* Pre-parsed evaluation is sub-microsecond for numeric expressions and about
  4.4 microseconds for the string concatenation case.
* Function-heavy expressions are the slowest to parse and to format.
