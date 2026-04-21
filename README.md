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

The benchmark suite covers parse, evaluate, repeated evaluate, format,
traverse, and parse + evaluate workloads for representative expressions
including arithmetic, function-heavy, variable-heavy, and long-string cases.

### Sample Benchmark Report

The following results were collected on April 21, 2026 from a local debug
build. They are useful as a relative baseline only; absolute timings will be
lower in an optimized release build.

| Workload | Parse CPU | Evaluate CPU | Repeated Eval Throughput | Format CPU | Traverse CPU | Parse + Evaluate CPU |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Simple arithmetic | 6.3 us | 312 ns | 1.84M eval/s | 3.5 us | 312 ns | 9.4 us |
| Nested precedence | 7.8 us | 469 ns | 2.45M eval/s | 3.1 us | 469 ns | 5.2 us |
| Function-heavy | 31.2 us | 469 ns | 3.67M eval/s | 6.1 us | 469 ns | 26.2 us |
| Variable-heavy | 9.7 us | 312 ns | 2.05M eval/s | 2.6 us | 262 ns | 9.7 us |
| Long string | 7.8 us | 4.4 us | 204.8K eval/s | 3.5 us | 174 ns | 9.7 us |

Observed patterns:

* Parsing still dominates one-shot execution cost for every workload in this
  debug run.
* The explicit repeated-evaluation benchmark shows the benefit of parsing once:
  throughput ranges from about 1.84M to 3.67M evaluations per second for the
  numeric cases.
* Pre-parsed evaluation is sub-microsecond for numeric expressions and about
  4.4 microseconds for the string concatenation case.
* Function-heavy expressions remain the slowest to parse, while long-string
  evaluation remains the slowest repeated-evaluation case.
