#include "express/function.h"

#include "express/express.h"

#include <algorithm>
#include <functional>
#define _USE_MATH_DEFINES
#include <math.h>

namespace expression {

namespace functions {

// simple functions

inline bool value_is_null(double x) {
  return abs(x) < Value::kPrecision;
}

inline bool value_to_bool(double x) {
  return !value_is_null(x);
}

inline double bool_to_value(bool b) {
  return b ? 1.0 : 0.0;
}

static double sign(double x) {
  if (value_is_null(x))
    return 0.0;
  else if (x > 0.0)
    return 1.0;
  else
    return -1.0;
}

static double abs_(double x) {
  return abs(x);
}

static double not_(double x) {
  return bool_to_value(!value_to_bool(x));
}

static double xor_(double x, double y) {
  bool a = value_to_bool(x);
  bool b = value_to_bool(y);
  return bool_to_value(a ^ b);
}

// binary functions

template <class T>
struct bin_min {
  T operator()(const T& left, const T& right) const {
    return left < right ? left : right;
  }
};

template <class T>
struct bin_max {
  T operator()(const T& left, const T& right) const {
    return left < right ? right : left;
  }
};

class if_fun : public Function {
 public:
  if_fun() : Function("If", 3) {}

  virtual void Calculate(const Expression& expr,
                         int pos,
                         Value& val,
                         void* data) {
    int p1 = expr.buffer.read<int>(pos);
    int p2 = expr.buffer.read<int>(pos);
    int p3 = expr.buffer.read<int>(pos);

    Value cond;
    expr.CalculateNode(p1, cond, data);

    int cond_pos = cond ? p2 : p3;
    expr.CalculateNode(cond_pos, val, data);
  }
};

Function* find_default_function(const char* name) {
  static multi_fun<std::logical_or<Value> > logical_or_fun("Or");
  static multi_fun<std::logical_and<Value> > logical_and_fun("And");
  static multi_fun<bin_min<Value> > min_fun("Min");
  static multi_fun<bin_max<Value> > max_fun("Max");
  static math1_fun abs_fun("Abs", abs_);
  static math1_fun not_fun("Not", not_);
  static math1_fun sign_fun("Sign", sign);
  static math1_fun sqrt_fun("Sqrt", sqrt);
  static math1_fun sin_fun("Sin", sin);
  static math1_fun cos_fun("Cos", cos);
  static math1_fun tan_fun("Tan", tan);
  static math1_fun asin_fun("ASin", asin);
  static math1_fun acos_fun("ACos", acos);
  static math1_fun atan_fun("ATan", atan);
  static math2_fun atan2_fun("ATan2", atan2);
  static math2_fun bitxor_fun("BitXor", xor_);
  static if_fun _if;

  static Function* list[] = {&logical_or_fun,
                             &logical_and_fun,
                             &min_fun,
                             &max_fun,
                             &abs_fun,
                             &not_fun,
                             &sign_fun,
                             &sqrt_fun,
                             &sin_fun,
                             &cos_fun,
                             &tan_fun,
                             &asin_fun,
                             &acos_fun,
                             &atan_fun,
                             &atan2_fun,
                             &bitxor_fun,
                             &_if,
                             NULL};

  return find_function_list(list, name);
}

}  // namespace functions

}  // namespace expression
