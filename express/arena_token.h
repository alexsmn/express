#pragma once

#include <type_traits>

namespace expression {

// Arena tokens are copied into allocator-owned raw memory and are never
// individually destroyed, so they must remain trivial wrappers.
template <class BasicToken>
inline constexpr bool kIsArenaToken =
    std::is_trivially_copyable_v<BasicToken> &&
    std::is_trivially_destructible_v<BasicToken>;

template <class BasicToken>
inline constexpr void AssertArenaToken() {
  static_assert(
      kIsArenaToken<BasicToken>,
      "BasicToken must be a trivially copyable, trivially destructible "
      "arena token because expression stores tokens in allocator-backed raw "
      "memory without running destructors. Use a lightweight wrapper such as "
      "PolymorphicToken instead of owning or non-trivial token types.");
}

}  // namespace expression
