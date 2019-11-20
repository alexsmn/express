#include "express/parser.h"

#include "express/allocator.h"
#include "express/function.h"
#include "express/lexer.h"
#include "express/parser_delegate.h"

namespace expression {

class OperUna : public Token {
 public:
  OperUna(char oper, Token* operand) : oper_{oper}, operand_{operand} {}

  virtual Value Calculate(void* data) const override {
    auto val = operand_->Calculate(data);
    switch (oper_) {
      case '-':
        val = -val;
        break;
      case '!':
        val = !val;
        break;
      default:
        assert(false);
        break;
    }
    return val;
  }

  virtual void Traverse(TraverseCallback callb, void* param) const override {
    callb(this, param);
    callb(operand_, param);
  }

  virtual void Format(const FormatterDelegate& delegate,
                      std::string& str) const override {
    str += oper_;
    operand_->Format(delegate, str);
  }

 private:
  const char oper_;
  Token* operand_;
};

class OperBin : public Token {
 public:
  OperBin(char oper, Token* left, Token* right)
      : oper_{oper}, left_{left}, right_{right} {}

  virtual Value Calculate(void* data) const override {
    auto val = left_->Calculate(data);
    auto rval = right_->Calculate(data);

    switch (oper_) {
      case '+':
        val += rval;
        break;
      case '-':
        val -= rval;
        break;
      case '*':
        val *= rval;
        break;
      case '/':
        val /= rval;
        break;
      case '^':
        val = pow((double)val, (double)rval);
        break;
      case '=':
        val = val == rval;
        break;
      case '<':
        val = val < rval;
        break;
      case '>':
        val = val > rval;
        break;
      case 'l':
        val = val <= rval;
        break;
      case 'g':
        val = val >= rval;
        break;
      default:
        assert(false);
        break;
    }

    return val;
  }

  virtual void Traverse(TraverseCallback callb, void* param) const override {
    callb(this, param);
    callb(left_, param);
    callb(right_, param);
  }

  virtual void Format(const FormatterDelegate& delegate,
                      std::string& str) const override {
    left_->Format(delegate, str);
    str += ' ';
    switch (oper_) {
      case 'l':
        str += "<=";
        break;
      case 'g':
        str += ">=";
        break;
      default:
        str += oper_;
        break;
    }
    str += ' ';
    right_->Format(delegate, str);
  }

 private:
  const char oper_;
  Token* left_;
  Token* right_;
};  // namespace expression

class LexStr : public Token {
 public:
  LexStr(std::string_view str, Allocator& allocator)
      : len_{static_cast<int>(str.size())},
        str_{static_cast<char*>(allocator.allocate(len_))} {
    memcpy(str_, str.data(), len_);
  }

  virtual Value Calculate(void* data) const override {
    return Value{str_, len_};
  }

  virtual void Traverse(TraverseCallback callb, void* param) const override {
    callb(this, param);
  }

  virtual void Format(const FormatterDelegate& delegate,
                      std::string& str) const override {
    str += '"';
    str.append(str_, len_);
    str += '"';
  }

 private:
  const int len_;
  char* str_;
};

class LexLP : public Token {
 public:
  explicit LexLP(Token* oper) : oper_{oper} {}

  virtual Value Calculate(void* data) const override {
    return oper_->Calculate(data);
  }

  virtual void Traverse(TraverseCallback callb, void* param) const override {
    callb(this, param);
    callb(oper_, param);
  }

  virtual void Format(const FormatterDelegate& delegate,
                      std::string& str) const override {
    str += '(';
    oper_->Format(delegate, str);
    str += ')';
  }

 private:
  Token* oper_;
};

Parser::Parser(Lexer& lexer, Allocator& allocator, ParserDelegate& delegate)
    : lexer_{lexer}, allocator_{allocator}, delegate_{delegate} {}

Token* Parser::CreatePrimaryToken() {
  auto lexem = next_lexem_;
  ReadLexem();

  if (lexem.type & OPER_UNA) {
    assert(!(lexem.lexem & LEX_UNA));
    return CreateToken<OperUna>(allocator_, lexem.lexem, CreatePrimaryToken());
  }

  switch (lexem.lexem) {
    case LEX_NAME:
      if (next_lexem_.lexem == expression::LEX_LP) {
        // function
        const auto* fun = delegate_.FindFunction(lexem._string);
        if (!fun) {
          throw std::runtime_error{std::string{"function was not found: "} +
                                   std::string{next_lexem_._string}};
        }
        // read parameters
        expression::Token* params[256];
        unsigned int nparam = 0;
        ReadLexem();
        if (next_lexem_.lexem != expression::LEX_RP) {
          for (;;) {
            params[nparam++] = CreateBinaryOperator();
            if (next_lexem_.lexem != expression::LEX_COMMA)
              break;
            ReadLexem();
          }
          if (next_lexem_.lexem != expression::LEX_RP)
            throw std::runtime_error("missing ')'");
        }
        ReadLexem();
        if (fun->params != -1 && fun->params != nparam) {
          throw std::runtime_error{std::string{"parameters expected: "} +
                                   std::to_string(fun->params)};
        }
        return fun->CreateToken(allocator_, params, nparam);
      }
      break;

    case LEX_DBL:
      return CreateToken<ValueToken<double>>(allocator_, lexem._double);
    case LEX_STR:
      return CreateToken<LexStr>(allocator_, lexem._string, allocator_);
    case LEX_LP: {
      Token* pos2 = CreateBinaryOperator();
      if (next_lexem_.lexem != LEX_RP)
        throw std::runtime_error("missing ')'");
      ReadLexem();
      return CreateToken<LexLP>(allocator_, pos2);
    }
  }

  Token* pos = delegate_.CreateToken(allocator_, lexem, *this);
  if (pos)
    return pos;

  throw std::runtime_error("unexpected primary token");
}

Token* Parser::CreateBinaryOperator(int priority) {
  Token* left = CreatePrimaryToken();
  while (next_lexem_.type & OPER_BIN && next_lexem_.priority >= priority) {
    char oper = next_lexem_.lexem;
    char priority2 = next_lexem_.priority;
    ReadLexem();
    Token* right = CreateBinaryOperator(priority2 + 1);
    // Write operator
    left = CreateToken<OperBin>(allocator_, oper, left, right);
  }
  return left;
}

Token* Parser::Parse() {
  ReadLexem();

  Token* root = CreateBinaryOperator();
  if (next_lexem_.lexem != LEX_END)
    return nullptr;

  return root;
}

void Parser::ReadLexem() {
  next_lexem_ = lexer_.ReadLexem();
}

}  // namespace expression
