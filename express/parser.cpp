#include "express/parser.h"

#include "express/allocator.h"
#include "express/function.h"
#include "express/lexer.h"
#include "express/parser_delegate.h"
#include "express/standard_tokens.h"

namespace expression {

Parser::Parser(Lexer& lexer, Allocator& allocator, ParserDelegate& delegate)
    : lexer_{lexer}, allocator_{allocator}, delegate_{delegate} {}

Token* Parser::CreatePrimaryToken() {
  auto lexem = next_lexem_;
  ReadLexem();

  if (lexem.type & OPER_UNA) {
    assert(!(lexem.lexem & LEX_UNA));
    return CreateToken<UnaryOperatorToken>(allocator_, lexem.lexem,
                                           CreatePrimaryToken());
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
      return CreateToken<StringValueToken>(allocator_, lexem._string,
                                           allocator_);
    case LEX_LP: {
      Token* inner_token = CreateBinaryOperator();
      if (next_lexem_.lexem != LEX_RP)
        throw std::runtime_error("missing ')'");
      ReadLexem();
      return CreateToken<ParenthesesToken>(allocator_, inner_token);
    }
  }

  Token* token = delegate_.CreateToken(allocator_, lexem, *this);
  if (token)
    return token;

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
    left = CreateToken<BinaryOperatorToken>(allocator_, oper, left, right);
  }
  return left;
}

std::optional<PolymorphicToken> Parser::Parse() {
  ReadLexem();

  const Token* root_token = CreateBinaryOperator();
  if (next_lexem_.lexem != LEX_END)
    return std::nullopt;

  assert(root_token);
  return PolymorphicToken{*root_token};
}

void Parser::ReadLexem() {
  next_lexem_ = lexer_.ReadLexem();
}

}  // namespace expression
