#pragma once

#include "express/buffer.h"
#include "express/express.h"
#include "express/express_export.h"

namespace expression {

class Expression;

class EXPRESS_EXPORT Parser {
 public:
  Parser(Expression& expression, const char* buffer);

  bool ReadNumber();
  void ReadString();
  void ReadLexem();

  // returns primary expression position in buffer
  int expr_prim();

  // returns binary expression position in buffer
  int expr_bin(char prior = 0);

  int Parse();

  const char*	buf;
  char		lexem;
  double		_double;
  int			_int;
  const char*	_str;
  int			_strl;
  void*		_data;
  int			prior;
  int			type;
  Expression*	expr;
};

void WriteNumber(Buffer& buffer, double val);

} // namespace expression