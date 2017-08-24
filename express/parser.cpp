#include "express/parser.h"

namespace expression {

void WriteNumber(Buffer& buffer, double val) {
  if (val == (int)val) {
    int	num = (int)val;
    // 1 bit for sign
    if (!(num >> 7))
      buffer << (char)LEX_I1 << (char)num;
    else if (!(num >> 15))
      buffer << (char)LEX_I2 << (short)num;
    else
      buffer << (char)LEX_I4 << num;
  } else {
    buffer << (char)LEX_DBL << val;
  }
}

Parser::Parser(Expression& expression, const char* buffer)
    : expr(&expression),
      buf(buffer) {
}

bool Parser::ReadNumber() {
  bool res = false;
  double num = 0;
  double exp = 0;
  for (;;) {
    if (*buf == '.') {
      // check second dot
      if (exp)
        throw std::runtime_error("bad number");
      exp = 1;
    } else {
      // read next digit
      int digit = int(*buf) - int('0');
      if (digit < 0 || digit > 9)
        break;
      num = num*10 + digit;
      exp *= 10;
      res = true;
    }
    buf++;
  }
  if (!res)
    return false;
  if (exp)
    _double = num/exp;
  else
    _double = num;
  lexem = LEX_DBL;
  return true;
}

void Parser::ReadString() {
  assert(*buf == '"');
  buf++;
  _str = buf;
  while (*buf != '"') {
    if (!*buf)
      throw std::runtime_error("unterminated string");
    buf++;
  }
  _strl = (int)(buf - _str);
  buf++;
}

void Parser::ReadLexem()
{
repeat:
  type = 0;
  switch (*buf) {
  case ' ':
  case '\t':
  case '\n':
    buf++;
    goto repeat;
  case '\0':
  case '(':
  case ')':
    lexem = *buf++;
    return;
  case '!':
    lexem = *buf++, type = OPER_UNA;
    return;
  case '=':
    lexem = *buf++, type = OPER_BIN, prior = 0;
    return;
  case '>':
    lexem = *buf++, type = OPER_BIN, prior = 0;
    if (*buf == '=')
      lexem = LEX_GE, buf++;
    return;
  case '<':
    lexem = *buf++, type = OPER_BIN, prior = 0;
    if (*buf == '=')
      lexem = LEX_LE, buf++;
    return;
  case '-':
    lexem = *buf++, type = OPER_BIN|OPER_UNA, prior = 1;
    return;
  case '+':
    lexem = *buf++, type = OPER_BIN, prior = 1;
    return;
  case '*':
  case '/':
    lexem = *buf++, type = OPER_BIN, prior = 2;
    return;
  case '^':
    lexem = *buf++, type = OPER_BIN, prior = 3;
    return;
  case '"':
    lexem = LEX_STR;
    ReadString();
    return;
  default:
    if (!(expr->flags&EXPR_CUSTOM_NUM))
      if (ReadNumber())
        return;
    expr->ReadLexem(*this);
    return;
  }
}

int Parser::expr_prim() {
  if (type&OPER_UNA) {
    assert(!(lexem&LEX_UNA));
    char oper = lexem;
    ReadLexem();
    int pos2 = expr_prim();
    int pos = expr->buffer.size;
    expr->buffer << char(oper|LEX_UNA) << pos2;
    return pos;
  }

  int pos = expr->WriteLexem(*this);
  if (pos != EXPR_DEF)
    return pos;

  switch (lexem) {
    case LEX_DBL: {
      int pos = expr->buffer.size;
      WriteNumber(expr->buffer, _double);
      ReadLexem();
      return pos;
    }
    case LEX_STR: {
      int pos = expr->buffer.size;
      expr->buffer << (char)LEX_STR;
      if (_strl > 0)
        expr->buffer.write(_str, _strl);
      expr->buffer << '\0';
      ReadLexem();
      return pos;
    }
    case LEX_LP: {
      ReadLexem();
      int pos2 = expr_bin();
      if (lexem != LEX_RP)
        throw std::runtime_error("missing ')'");
      int pos = expr->buffer.size;
      expr->buffer << (char)LEX_LP << pos2;
      ReadLexem();
      return pos;
    }
    default:
      throw std::runtime_error("unexpected primary token");
  }
}

int Parser::expr_bin(char prior) {
  int left = expr_prim();
  while (type&OPER_BIN && this->prior >= prior) {
    char oper = this->lexem;
    char prior2 = this->prior;
    ReadLexem();
    int right = expr_bin(prior2);
    // Write operator
    int pos = expr->buffer.size;
    expr->buffer << (char)oper << left << right;
    left = pos;
  }
  return left;
}

int Parser::Parse() {
  ReadLexem();

  int root = expr_bin();
  if (lexem != LEX_END)
    return -1;

  return root;
}

} // namespace expression
