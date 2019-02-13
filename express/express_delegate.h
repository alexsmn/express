#pragma once

#include "express/express_export.h"
#include "express/lexem_data.h"

namespace expression {

class Buffer;
class Parser;

struct ReadBuffer {
  const char*& buf;
};

class EXPRESS_EXPORT ExpressionDelegate {
 public:
  virtual ~ExpressionDelegate() {}

  // should fill lexem of throw exception
  virtual LexemData ReadLexem(ReadBuffer& buffer) {
    throw std::runtime_error("ReadLexem");
  }

  // return written lexem position or EXPR_DEF for default handler
  virtual int WriteLexem(const LexemData& lexem_data,
                         Parser& parser,
                         Buffer& buffer) {
    return EXPR_DEF;
  }

  virtual void CalculateLexem(const expression::Buffer& buffer,
                              int pos,
                              Lexem lexem,
                              Value& val,
                              void* data) const {
    throw std::runtime_error("CalculateLexem");
  }

  virtual std::string FormatLexem(const expression::Buffer& buffer,
                                  int pos,
                                  Lexem lexem) const {
    throw std::runtime_error("FormatLexem");
  }

  virtual void TraverseLexem(const Expression& expr,
                             int pos,
                             Lexem lexem,
                             TraverseCallback callb,
                             void* param) const {
    callb(expr, lexem, param);
  }

  virtual Function* FindFunction(const char* name);
};

}  // namespace expression
