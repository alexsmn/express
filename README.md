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
short-circuit boolean, folded variadic-function, heavier string
concatenation, and default-vs-generic execution-path comparisons.

### Sample Benchmark Report

The following results were collected on April 21, 2026 from a local debug
build. They are useful as a relative baseline only; absolute timings will be
lower in an optimized release build.

| Workload | Parse CPU | Evaluate CPU | Repeated Eval Throughput | Format CPU | Traverse CPU | Parse + Evaluate CPU |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Simple arithmetic | 9.7 us | 781 ns | 1.83M eval/s | 3.1 us | 312 ns | 9.6 us |
| Nested precedence | 7.8 us | 625 ns | 2.05M eval/s | 3.1 us | 469 ns | 9.4 us |
| Function-heavy | 31.3 us | 698 ns | 1.37M eval/s | 7.0 us | 973 ns | 46.9 us |
| Variable-heavy | 19.5 us | 785 ns | 1.37M eval/s | 1.7 us | 312 ns | 19.5 us |
| Long string | 9.4 us | 3.1 us | 657.4K eval/s | 2.6 us | 174 ns | 7.0 us |
| String concat heavy | 31.3 us | 3.1 us | 204.8K eval/s | 4.7 us | 625 ns | 31.2 us |

Reserve-path parse measurements from the same debug run:

| Workload | Parse Reserved CPU | Parse + Evaluate Reserved CPU |
| --- | ---: | ---: |
| Simple arithmetic | 9.4 us | 7.8 us |
| Nested precedence | 7.0 us | 5.4 us |
| Function-heavy | 31.2 us | 26.2 us |
| Variable-heavy | 9.7 us | 7.8 us |
| Long string | 5.2 us | 4.7 us |
| String concat heavy | 14.6 us | 17.4 us |

Short-circuit boolean-chain evaluation:

| Workload | Evaluate CPU |
| --- | ---: |
| `Or(1, 0, ..., 0)` | 262 ns |
| `And(0, 1, ..., 1)` | 312 ns |

Folded variadic-function measurements from the same debug run:

| Workload | Parse CPU | Evaluate CPU | Traverse CPU |
| --- | ---: | ---: | ---: |
| `Min(9, 4, 6, 8, 3, 10, 2, 7)` | 17.4 us | 1.09 us | 469 ns |
| `Or(0, 0, 0, 0, 0, 0, 1, 0)` | 17.5 us | 1.25 us | 781 ns |

Default-path vs generic-path repeated evaluation on the same default-parser subset:

| Workload | Generic Path Throughput | Default Path Throughput |
| --- | ---: | ---: |
| Simple arithmetic | 1.47M eval/s | 1.22M eval/s |
| Nested precedence | 2.05M eval/s | 1.37M eval/s |
| Function-heavy | 2.05M eval/s | 1.22M eval/s |
| Long string | 819.2K eval/s | 512.0K eval/s |
| String concat heavy | 204.8K eval/s | 204.8K eval/s |

Observed patterns:

* Parsing still dominates one-shot execution cost for every workload in this
  debug run.
* Reserved parse capacity helps some shapes noticeably, especially nested
  precedence, function-heavy, and some string-heavy runs, while other shapes remain
  roughly flat in a debug build.
* The explicit repeated-evaluation benchmark shows the benefit of parsing once:
  throughput ranges from about 1.37M to 2.05M evaluations per second for the
  numeric cases in this run.
* Pre-parsed evaluation is sub-microsecond for numeric expressions and about
  3.1 microseconds for the string-heavy cases in this run.
* The dedicated short-circuit boolean cases are materially cheaper than the
  general function-heavy path after the `And`/`Or` fold switched to a
  right-associated short-circuit tree.
* Folding `Min`/`Max`/`And`/`Or` into binary trees removes array-backed
  variadic evaluation for the standard functions while keeping formatted output
  stable.
* Inline string storage materially reduces the cost of string-heavy evaluation:
  the two-literal string case runs at about 657K eval/s in this debug build.
* The new non-virtual default parser path is functionally correct and isolated
  from the generic customization path, but it is still slower than the generic
  token engine in this debug build. That remains the main unresolved
  optimization task.
* Function-heavy and string-concatenation-heavy expressions remain the slowest
  workloads in this benchmark suite.
