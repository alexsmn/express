#include "express/express.h"

#include "base/strings/string_number_conversions.h"
#include "express/express_delegate.h"
#include "express/function.h"
#include "express/parser.h"

namespace expression {

double Value::kPrecision = 1e-6;

void Expression::Parse(const char* buf, int flags) {
  assert(buffer.empty());
  assert(!buffer.cap);
  assert(!buffer.size);

  char temp_mem[4096];
  buffer.buf = temp_mem;
  buffer.cap = sizeof(temp_mem);

  try {
    Parser parser{*this, buf, flags};
    root = parser.Parse();
    if (root < 0)
      throw std::runtime_error("expression expected");

  } catch (...) {
    buffer.cap = 0;
    buffer.buf = 0;
    buffer.size = 0;
    throw;
  }

  assert(buffer.size > 0);
  void* mem = malloc(buffer.size);
  memcpy(mem, temp_mem, buffer.size);
  buffer.buf = (char*)mem;
  buffer.cap = buffer.size;
}

Value Expression::Calculate(void* data) const {
  Value value;
  CalculateNode(root, value, data);
  return value;
}

void Expression::Clear() {
  if (buffer.cap)
    free(buffer.buf);
  buffer.cap = buffer.size = 0;
  buffer.buf = NULL;
}

void Expression::CalculateNode(int pos, Value& val, void* data) const {
  Lexem lexem = (Lexem)buffer.read<char>(pos);
  switch ((unsigned char)lexem) {
    case LEX_DBL:
      val = buffer.read<double>(pos);
      return;
    case LEX_I4:
      val = buffer.read<int>(pos);
      return;
    case LEX_I2:
      val = buffer.read<short>(pos);
      return;
    case LEX_I1:
      val = buffer.read<char>(pos);
      return;
    case LEX_STR: {
      const char* str = buffer.ptr(pos);
      const char* p = str;
      while (*p)
        p++;
      size_t len = p - str;
      pos += len + 1;  // should be safe
      val.set_string(str, len);
      return;
    }
    case LEX_FUN: {
      Function* fun = buffer.read<Function*>(pos);
      fun->Calculate(*this, pos, val, data);
      return;
    }
    case '-' | LEX_UNA:
    case '!' | LEX_UNA: {
      int arg_pos = buffer.read<int>(pos);
      CalculateNode(arg_pos, val, data);
      switch ((unsigned char)(lexem & ~LEX_UNA)) {
        case '-':
          val = -val;
          return;
        case '!':
          val = !val;
          return;
        default:
          assert(false);
      }
    }
    case '+':
    case '-':
    case '*':
    case '/':
    case '^':
    case '=':
    case '>':
    case '<':
    case 'l':
    case 'g': {
      int left = buffer.read<int>(pos);
      int right = buffer.read<int>(pos);
      Value rval;
      CalculateNode(left, val, data);
      CalculateNode(right, rval, data);
      switch (lexem) {
        case '+':
          val += rval;
          return;
        case '-':
          val -= rval;
          return;
        case '*':
          val *= rval;
          return;
        case '/':
          val /= rval;
          return;
        case '^':
          val = pow((double)val, (double)rval);
          return;
        case '=':
          val = val == rval;
          return;
        case '<':
          val = val < rval;
          return;
        case '>':
          val = val > rval;
          return;
        case 'l':
          val = val <= rval;
          return;
        case 'g':
          val = val >= rval;
          return;
        default:
          assert(false);
          return;
      }
    }
    case LEX_LP: {
      int arg_pos = buffer.read<int>(pos);
      CalculateNode(arg_pos, val, data);
      return;
    }
    default:
      delegate_.CalculateLexem(buffer, pos, lexem, val, data);
      return;
  }
}

std::string Expression::FormatNode(int pos) const {
  Lexem lexem = (Lexem)buffer.read<char>(pos);
  switch ((unsigned char)lexem) {
    case LEX_DBL:
      return base::DoubleToString(buffer.read<double>(pos));
    case LEX_I4:
      return base::IntToString(buffer.read<int>(pos));
    case LEX_I2:
      return base::IntToString(buffer.read<short>(pos));
    case LEX_I1:
      return base::IntToString(buffer.read<char>(pos));
    case LEX_STR: {
      std::string str = buffer.ptr(pos);
      pos += str.length() + 1;
      str.insert(str.begin(), '"');
      str.insert(str.end(), '"');
      return str;
    }
    case '-' | LEX_UNA:
    case '!' | LEX_UNA: {
      char oper = lexem & ~LEX_UNA;
      int arg_pos = buffer.read<int>(pos);
      std::string str = FormatNode(arg_pos);
      str.insert(str.begin(), oper);
      return str;
    }
    case '+':
    case '-':
    case '*':
    case '/':
    case '^':
    case '=':
    case '>':
    case '<':
    case 'l':
    case 'g': {
      char oper = lexem;
      std::string str;
      int left = buffer.read<int>(pos);
      int right = buffer.read<int>(pos);
      str = FormatNode(left);
      switch (oper) {
        case 'l':
          str += "<=";
          break;
        case 'g':
          str += ">=";
          break;
        default:
          str += lexem;
          break;
      }
      str += FormatNode(right);
      return str;
    }
    case LEX_LP: {
      int arg_pos = buffer.read<int>(pos);
      std::string str = FormatNode(arg_pos);
      str.insert(str.begin(), '(');
      str.insert(str.end(), ')');
      return str;
    }
    default:
      return delegate_.FormatLexem(pos, lexem);
  }
}

void Expression::TraverseNode(int pos,
                              TraverseCallback callb,
                              void* param) const {
  Lexem lexem = (Lexem)buffer.read<char>(pos);
  switch ((unsigned char)lexem) {
    case LEX_DBL:
    case LEX_I4:
    case LEX_I2:
    case LEX_I1:
    case LEX_STR:
      callb(*this, lexem, param);
      return;
    case '-' | LEX_UNA:
    case '!' | LEX_UNA:
    case LEX_LP: {
      int arg_pos = buffer.read<int>(pos);
      callb(*this, lexem, param);
      TraverseNode(arg_pos, callb, param);
      return;
    }
    case '+':
    case '-':
    case '*':
    case '/':
    case '^':
    case '=':
    case '>':
    case '<':
    case 'l':
    case 'g': {
      int left = buffer.read<int>(pos);
      int right = buffer.read<int>(pos);
      callb(*this, lexem, param);
      TraverseNode(left, callb, param);
      TraverseNode(right, callb, param);
      return;
    }
    default:
      delegate_.TraverseLexem(*this, pos, lexem, callb, param);
      return;
  }
}

namespace functions {
extern Function* find_default_function(const char* name);
}

Function* ExpressionDelegate::FindFunction(const char* name) {
  return functions::find_default_function(name);
}

// Function

std::string Function::Format(const Expression& expr, int pos) const {
  int npar = this->params;
  if (npar == -1)
    npar = expr.buffer.read<char>(pos);

  std::string str = name;
  str.insert(str.end(), '(');

  int pars[60];
  for (int i = 0; i < npar; i++)
    pars[i] = expr.buffer.read<int>(pos);

  for (int i = 0; i < npar; i++) {
    str += expr.FormatNode(pars[i]);
    if (i < npar - 1)
      str.insert(str.end(), ',');
  }

  str.insert(str.end(), ')');

  return str;
}

void Function::Traverse(const Expression& expr,
                        int pos,
                        TraverseCallback callb,
                        void* param) const {
  int npar = this->params;
  if (npar == -1)
    npar = expr.buffer.read<char>(pos);

  int pars[60];
  for (int i = 0; i < npar; i++)
    pars[i] = expr.buffer.read<int>(pos);

  for (int i = 0; i < npar; i++)
    expr.TraverseNode(pars[i], callb, param);
}

}  // namespace expression
