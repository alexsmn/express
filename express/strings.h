#pragma once

#include <cctype>
#include <string_view>

namespace expression {

inline bool EqualsNoCase(std::string_view a, std::string_view b) {
  return std::equal(a.begin(), a.end(), b.begin(), b.end(), [](char a, char b) {
    return std::tolower(static_cast<unsigned char>(a)) ==
           std::tolower(static_cast<unsigned char>(b));
  });
}

}  // namespace expression
