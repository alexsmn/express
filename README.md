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
boolean-chain evaluate, folded-variadic parse/evaluate/traverse, format,
traverse, and parse + evaluate workloads for representative expressions
including arithmetic, function-heavy, variable-heavy, long-string,
short-circuit boolean, and folded variadic-function cases.

### Sample Benchmark Report

The following results were collected on April 21, 2026 from a local debug
build. They are useful as a relative baseline only; absolute timings will be
lower in an optimized release build.

| Workload | Parse CPU | Evaluate CPU | Repeated Eval Throughput | Format CPU | Traverse CPU | Parse + Evaluate CPU |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Simple arithmetic | 6.3 us | 312 ns | 2.05M eval/s | 3.1 us | 312 ns | 7.8 us |
| Nested precedence | 6.3 us | 469 ns | 2.05M eval/s | 3.1 us | 436 ns | 6.3 us |
| Function-heavy | 46.9 us | 312 ns | 2.44M eval/s | 7.8 us | 785 ns | 46.9 us |
| Variable-heavy | 9.7 us | 312 ns | 1.83M eval/s | 3.1 us | 262 ns | 14.6 us |
| Long string | 8.1 us | 3.5 us | 204.8K eval/s | 2.6 us | 156 ns | 14.6 us |

Reserve-path parse measurements from the same debug run:

| Workload | Parse Reserved CPU | Parse + Evaluate Reserved CPU |
| --- | ---: | ---: |
| Simple arithmetic | 6.3 us | 7.8 us |
| Nested precedence | 5.2 us | 6.1 us |
| Function-heavy | 26.2 us | 31.2 us |
| Variable-heavy | 8.1 us | 7.0 us |
| Long string | 7.8 us | 8.1 us |

Short-circuit boolean-chain evaluation:

| Workload | Evaluate CPU |
| --- | ---: |
| `Or(1, 0, ..., 0)` | 78.1 ns |
| `And(0, 1, ..., 1)` | 109 ns |

Folded variadic-function measurements from the same debug run:

| Workload | Parse CPU | Evaluate CPU | Traverse CPU |
| --- | ---: | ---: | ---: |
| `Min(9, 4, 6, 8, 3, 10, 2, 7)` | 17.4 us | 1.09 us | 625 ns |
| `Or(0, 0, 0, 0, 0, 0, 1, 0)` | 31.2 us | 543 ns | 625 ns |

Observed patterns:

* Parsing still dominates one-shot execution cost for every workload in this
  debug run.
* Reserved parse capacity helps some shapes noticeably, especially nested
  precedence, variable-heavy, and long-string runs, while other shapes remain
  roughly flat in a debug build.
* The explicit repeated-evaluation benchmark shows the benefit of parsing once:
  throughput ranges from about 1.83M to 2.44M evaluations per second for the
  numeric cases in this run.
* Pre-parsed evaluation is sub-microsecond for numeric expressions and about
  3.5 microseconds for the string concatenation case.
* The dedicated short-circuit boolean cases are materially cheaper than the
  general function-heavy path after the `And`/`Or` fold switched to a
  right-associated short-circuit tree.
* Folding `Min`/`Max`/`And`/`Or` into binary trees removes array-backed
  variadic evaluation for the standard functions while keeping formatted output
  stable.
* Function-heavy expressions remain the slowest to parse, while long-string
  evaluation remains the slowest repeated-evaluation case.
