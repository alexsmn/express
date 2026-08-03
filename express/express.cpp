#include "express/express.h"

#include "express/lexer.h"
#include "express/lexer_delegate.h"
#include "express/parser.h"
#include "express/parser_delegate.h"

namespace expression {

void Expression::Traverse(TraverseCallback callback, void* param) const {
  BasicExpression::Traverse(
      [callback, param](const Token* token) { return callback(token, param); });
}

}  // namespace expression
