#include "express/express.h"

#include "express/lexer.h"
#include "express/lexer_delegate.h"
#include "express/parser.h"
#include "express/parser_delegate.h"

namespace expression {

void Expression::Parse(const char* buf) {
  LexerDelegate lexer_delegate;
  Lexer lexer{buf, lexer_delegate, 0};
  Allocator allocator;
  BasicParserDelegate<Lexer, PolymorphicToken> parser_delegate;
  BasicParser<Lexer, PolymorphicToken> parser{lexer, allocator,
                                              parser_delegate};
  BasicExpression::Parse(parser, allocator);
}

void Expression::Traverse(TraverseCallback callback, void* param) const {
  BasicExpression::Traverse(
      [callback, param](const Token* token) { return callback(token, param); });
}

}  // namespace expression
