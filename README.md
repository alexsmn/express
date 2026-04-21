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

The benchmark suite covers parse, reserved parse, evaluate, repeated evaluate,
boolean-chain evaluate, format, traverse, and parse + evaluate workloads for
representative expressions including arithmetic, function-heavy, variable-heavy,
long-string, and short-circuit boolean cases.

### Sample Benchmark Report

The following results were collected on April 21, 2026 from a local debug
build. They are useful as a relative baseline only; absolute timings will be
lower in an optimized release build.

| Workload | Parse CPU | Evaluate CPU | Repeated Eval Throughput | Format CPU | Traverse CPU | Parse + Evaluate CPU |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Simple arithmetic | 6.1 us | 625 ns | 2.05M eval/s | 4.7 us | 312 ns | 7.8 us |
| Nested precedence | 6.3 us | 625 ns | 2.05M eval/s | 3.5 us | 469 ns | 3.1 us |
| Function-heavy | 26.2 us | 469 ns | 1.37M eval/s | 7.8 us | 469 ns | 31.3 us |
| Variable-heavy | 9.7 us | 469 ns | 2.05M eval/s | 4.7 us | 262 ns | 9.7 us |
| Long string | 9.7 us | 6.3 us | 136.5K eval/s | 2.6 us | 146 ns | 10.5 us |

Reserve-path parse measurements from the same debug run:

| Workload | Parse Reserved CPU | Parse + Evaluate Reserved CPU |
| --- | ---: | ---: |
| Simple arithmetic | 7.8 us | 6.1 us |
| Nested precedence | 4.7 us | 6.3 us |
| Function-heavy | 26.2 us | 26.2 us |
| Variable-heavy | 10.9 us | 9.4 us |
| Long string | 9.4 us | 6.3 us |

Short-circuit boolean-chain evaluation:

| Workload | Evaluate CPU |
| --- | ---: |
| `Or(1, 0, ..., 0)` | 174 ns |
| `And(0, 1, ..., 1)` | 125 ns |

Observed patterns:

* Parsing still dominates one-shot execution cost for every workload in this
  debug run.
* Reserved parse capacity helps some shapes noticeably, especially nested
  precedence and long-string parse + evaluate runs, while other shapes remain
  roughly flat in a debug build.
* The explicit repeated-evaluation benchmark shows the benefit of parsing once:
  throughput ranges from about 1.37M to 2.05M evaluations per second for the
  numeric cases in this run.
* Pre-parsed evaluation is sub-microsecond for numeric expressions and about
  6.3 microseconds for the string concatenation case.
* The dedicated short-circuit boolean cases are materially cheaper than the
  general function-heavy path because evaluation stops at the first decisive
  operand.
* Function-heavy expressions remain the slowest to parse, while long-string
  evaluation remains the slowest repeated-evaluation case.
