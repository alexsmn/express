#pragma once

#include "express/express.h"
#include "express/express_export.h"

#include <stdexcept>
#include <string>

namespace expression {

class Expression;

class EXPRESS_EXPORT Function {
 public:
  Function(const char* name, int params) : name(name), params(params) {}

  virtual void prepare(Expression& expr) {}
  virtual void Calculate(const Expression& expr,
                         int pos,
                         Value& val,
                         void* data) = 0;
  std::string Format(const Expression& expr, int pos) const;
  void Traverse(const Expression& expr,
                int pos,
                TraverseCallback callb,
                void* param) const;

  const char* name;
  int params;
};

inline bool EqualsNoCase(const char* a, const char* b) {
#ifdef OS_WIN
  return _strcmpi(a, b) == 0;
#else
  return strcasecmp(a, b) == 0;
#endif
}

inline Function* find_function_list(Function** list, const char* name) {
  for (; *list; ++list) {
    if (EqualsNoCase((*list)->name, name))
      return *list;
  }
  return NULL;
}

/*template<typename T>
class bin_fun : public Function
{
public:
  explicit bin_fun(const char* name) : Function(name, 2) { }

  virtual void Calculate(const Expression& expr, int pos, Value& val,
                         void* data) {
    int left, right;
    expr.buffer >> left >> right;
    Value lval, rval;
    expr.CalculateNode(left, lval, data);
    expr.CalculateNode(right, rval, data);
    val = T()(lval, rval);
  }
};*/

template <typename T>
class multi_fun : public Function {
 public:
  explicit multi_fun(const char* name) : Function(name, -1) {}

  virtual void Calculate(const Expression& expr,
                         int pos,
                         Value& val,
                         void* data) {
    int nparam = expr.buffer.read<unsigned char>(pos);
    if (nparam < 2)
      throw std::runtime_error("multi_fun::Calculate");

    int params[256];
    expr.buffer.read(pos, params, nparam * sizeof(int));

    expr.CalculateNode(params[0], val, data);

    Value pval;
    for (int i = 1; i < nparam; i++) {
      expr.CalculateNode(params[i], pval, data);
      val = T()(val, pval);
    }
  }
};

class math1_fun : public Function {
 public:
  typedef double (*fun_t)(double);
  fun_t fun;

  math1_fun(const char* name, fun_t fun) : Function(name, 1), fun(fun) {}

  virtual void Calculate(const Expression& expr,
                         int pos,
                         Value& val,
                         void* data) {
    int param_pos = expr.buffer.read<int>(pos);
    Value v;
    expr.CalculateNode(param_pos, v, data);
    val = fun(v);
  }
};

class math2_fun : public Function {
 public:
  typedef double (*fun_t)(double, double);
  fun_t fun;

  math2_fun(const char* name, fun_t fun) : Function(name, 2), fun(fun) {}

  virtual void Calculate(const Expression& expr,
                         int pos,
                         Value& val,
                         void* data) {
    int p1 = expr.buffer.read<int>(pos);
    int p2 = expr.buffer.read<int>(pos);

    Value v1, v2;
    expr.CalculateNode(p1, v1, data);
    expr.CalculateNode(p2, v2, data);

    val = fun(v1, v2);
  }
};

}  // namespace expression
