# Performance Implementation Checklist

## Phase 1: Formatter and Benchmark Baseline

Commit: `Optimize formatting hot path and expand benchmark baseline`

- [x] In [express/formatter_delegate.h](/mnt/d/tc/third_party/express/express/formatter_delegate.h), replace `std::to_string` in `AppendDouble` with a `std::to_chars`-based implementation.
- [x] Preserve current public API and keep a fallback only if floating-point `to_chars` is unavailable.
- [x] In [benchmarks/benchmark.cpp](/mnt/d/tc/third_party/express/benchmarks/benchmark.cpp), add repeated-evaluation benchmarks for pre-parsed expressions.
- [x] Keep the current benchmark workload matrix and add only the new repeated-eval dimension.
- [x] Re-run benchmarks and refresh the benchmark section in [README.md](/mnt/d/tc/third_party/express/README.md).
- [x] Verify unit tests still pass unchanged.

Exit criteria:

- [x] `Format` benchmarks improve or at minimum do not regress.
- [x] README reflects the new benchmark shape.

## Phase 2: Allocator Reserve Hook

Commit: `Add allocator reserve support for parse-heavy workloads`

- [x] In [express/allocator.h](/mnt/d/tc/third_party/express/express/allocator.h), add a `reserve_bytes(size_t, size_t alignment = alignof(std::max_align_t))` API.
- [x] Keep `allocate()` semantics unchanged for existing callers.
- [x] In [express/basic_expression.h](/mnt/d/tc/third_party/express/express/basic_expression.h), add a parse path that pre-reserves allocator space using a formula-length heuristic.
- [x] Keep `Parse(const char*)` source-compatible and make reserve automatic only for the default parse path.
- [x] In [benchmarks/benchmark.cpp](/mnt/d/tc/third_party/express/benchmarks/benchmark.cpp), add parse and parse+evaluate measurements for the reserved path.
- [x] In [tests/test.cpp](/mnt/d/tc/third_party/express/tests/test.cpp), add behavioral tests proving reserve does not change parse/evaluate results.

Exit criteria:

- [x] Parse-heavy benchmarks improve for medium/large inputs.
- [x] No public API break beyond additive allocator functionality.

## Phase 3: Short-Circuit Boolean Execution

Commit: `Implement short-circuit And/Or evaluation`

- [x] In [express/standard_functions.h](/mnt/d/tc/third_party/express/express/standard_functions.h), replace generic variadic reduction for `And` and `Or` with dedicated token implementations.
- [x] Evaluate operands left-to-right and stop on the first decisive operand.
- [x] Preserve current truthiness semantics from `Value::operator bool()`.
- [x] Leave `Min` and `Max` on the generic variadic implementation in this phase.
- [x] In [tests/test.cpp](/mnt/d/tc/third_party/express/tests/test.cpp), add regression tests using custom tokens that throw on unexpected evaluation to prove short-circuiting.
- [x] In [benchmarks/benchmark.cpp](/mnt/d/tc/third_party/express/benchmarks/benchmark.cpp), add long boolean-chain benchmark cases.

Exit criteria:

- [x] Boolean-chain eval benchmarks improve.
- [x] Tests prove both `And` and `Or` short-circuit correctly.

## Phase 4: Fold Variadic Functions into Binary Trees

Commit: `Fold standard variadic functions into binary parse trees`

- [x] In [express/parser_delegate.h](/mnt/d/tc/third_party/express/express/parser_delegate.h), add support for building folded binary forms for standard variadic functions.
- [x] In [express/standard_functions.h](/mnt/d/tc/third_party/express/express/standard_functions.h), implement binary folding for `Min`, `Max`, `And`, and `Or`.
- [x] Preserve left-to-right associativity and existing formatting output.
- [x] Keep the generic variadic implementation available for future custom functions.
- [x] In [tests/test.cpp](/mnt/d/tc/third_party/express/tests/test.cpp), add coverage proving evaluation and formatting remain unchanged after folding.
- [x] Add optional traversal/token-shape assertions if useful.
- [x] Rebaseline parse, evaluate, and traverse benchmarks for the folded functions.

Exit criteria:

- [x] Standard variadic functions no longer depend on array-backed hot-path storage.
- [x] Benchmarks show improvement or at minimum reduced memory overhead.

## Phase 5: Reduce String Allocation Cost in `Value`

Commit: `Add inline string storage to Value`

- [x] In [express/value.h](/mnt/d/tc/third_party/express/express/value.h), redesign string storage to support small-string optimization.
- [x] Keep constructors, conversions, comparisons, concatenation, and assignment source-compatible.
- [x] Preserve stable ownership semantics for computed strings.
- [x] In [express/standard_tokens.h](/mnt/d/tc/third_party/express/express/standard_tokens.h), ensure parsed string literals benefit from the new representation with minimal copying.
- [x] In [tests/test.cpp](/mnt/d/tc/third_party/express/tests/test.cpp), add coverage for:
  - [x] short literal strings
  - [x] long literal strings
  - [x] short+short concat
  - [x] short+long concat
  - [x] equality and ordering
  - [x] self-assignment across inline and heap-backed thresholds
- [x] In [benchmarks/benchmark.cpp](/mnt/d/tc/third_party/express/benchmarks/benchmark.cpp), add stronger string-heavy workloads and refresh results.

Exit criteria:

- [x] String-heavy evaluation improves significantly.
- [x] Existing `Value` behavior remains intact.

## Phase 6: Specialize the Default Engine

Commit: `Add non-virtual fast path for default Expression`

- [ ] In [express/express.h](/mnt/d/tc/third_party/express/express/express.h) and [express/express.cpp](/mnt/d/tc/third_party/express/express/express.cpp), introduce an internal default-engine representation optimized for the standard parser path.
- [ ] Keep the public `Expression` API unchanged.
- [ ] In [express/token.h](/mnt/d/tc/third_party/express/express/token.h), keep `Token` and `PolymorphicToken` as the customization surface; do not remove them.
- [ ] In [express/standard_tokens.h](/mnt/d/tc/third_party/express/express/standard_tokens.h), implement a compact internal representation for default expressions, starting with literals, arithmetic, comparison, parentheses, and current standard functions.
- [ ] Route only `Expression::Parse(const char*)` through the fast path.
- [ ] Keep custom parser/delegate usage on the generic token path.
- [ ] In [tests/test.cpp](/mnt/d/tc/third_party/express/tests/test.cpp), ensure the existing full behavior suite still passes unchanged.
- [ ] Add comparison tests between default-path and generic-path evaluation/format results.
- [ ] In [benchmarks/benchmark.cpp](/mnt/d/tc/third_party/express/benchmarks/benchmark.cpp), report default-path vs generic-path performance.

Exit criteria:

- [ ] Default-engine repeated evaluation is materially faster.
- [ ] Customization behavior remains intact and separate.

## Recommended Gate Between Phases

- [x] Do not start Phase 4 until Phase 3 benchmarks confirm short-circuit improvements.
- [x] Do not start Phase 6 until Phase 5 benchmark gains are measured; otherwise it will be hard to attribute wins.
- [ ] Refresh README benchmark numbers only after Phases 1, 3, 5, and 6, not after every phase.

## Suggested Final Validation Pass

Commit: `Refresh benchmark report after performance improvements`

- [ ] Run the full benchmark suite in the intended build mode.
- [ ] Update the benchmark section in [README.md](/mnt/d/tc/third_party/express/README.md).
- [ ] Verify all unit tests pass.
- [ ] Confirm no machine-specific paths or environment-specific wording leaked into documentation.
