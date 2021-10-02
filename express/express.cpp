#include "express/express.h"

#include "express/lexer.h"
#include "express/parser.h"

namespace expression {

void Expression::Parse(const char* buf,
                       LexerDelegate& lexer_delegate,
                       ParserDelegate& parser_delegate,
                       int flags) {
  Lexer lexer{buf, lexer_delegate, flags};
  Allocator allocator;
  Parser parser{lexer, allocator, parser_delegate};
  BasicExpression::Parse(parser, allocator);
}

}  // namespace expression
