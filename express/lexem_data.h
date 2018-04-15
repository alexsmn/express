#pragma once

namespace expression {

struct LexemData {
  char lexem;
  double _double;
  int _int;
  const char* _str;
  int _strl;
};

} // namespace expression
