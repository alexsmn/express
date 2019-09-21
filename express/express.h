#pragma once

#include "express/allocator.h"
#include "express/express_export.h"
#include "express/token.h"
#include "express/value.h"

namespace expression {

class FormatterDelegate;
class LexerDelegate;
class ParserDelegate;
class Token;

class EXPRESS_EXPORT Expression {
 public:
  Expression() {}
  ~Expression() { Clear(); }

  Expression(const Expression&) = delete;
  Expression& operator=(const Expression&) = delete;

  void Parse(const char* buf,
             LexerDelegate& lexer_delegate,
             ParserDelegate& parser_delegate,
             int flags = 0);

  Value Calculate(void* data = NULL) const;

  void Traverse(TraverseCallback callb, void* param) const;

  std::string Format(const FormatterDelegate& delegate) const;

  void Clear();

  void swap(Expression& other) {
    allocator_.swap(other.allocator_);
    std::swap(root_, other.root_);
  }

  Allocator allocator_;
  Token* root_ = nullptr;
};

}  // namespace expression
