#pragma once

#include "express/lexem.h"

#include <optional>

namespace expression {

struct ReadBuffer {
  const char*& buf;
};

class EXPRESS_EXPORT LexerDelegate {
 public:
  virtual ~LexerDelegate() {}

  virtual std::optional<Lexem> ReadLexem(ReadBuffer& buffer) {
    return std::nullopt;
  }
};

}  // namespace expression