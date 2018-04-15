#pragma once

#include "express/buffer.h"
#include "express/express.h"
#include "express/express_export.h"
#include "express/lexem_data.h"

namespace expression {

class Expression;

class EXPRESS_EXPORT Parser {
 public:
  Parser(Expression& expression, const char* buffer, int flags);

  Parser(const Parser&) = delete;
  Parser& operator=(const Parser&) = delete;

  bool ReadNumber();
  void ReadString();
  void ReadLexem();

  // returns primary expression position in buffer
  int expr_prim();

  // returns binary expression position in buffer
  int expr_bin(char prior = 0);

  int Parse();

  LexemData lexem_data_;

  const char*	buf;
  void*		_data;
  int			prior;
  int			type;
  Expression*	expr;
  const int flags_;
};

void WriteNumber(Buffer& buffer, double val);

} // namespace expression