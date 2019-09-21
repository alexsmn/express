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

template <class T>
struct Min {
  T operator()(T a, T b) const { return std::min(a, b); }
};

template <class T>
struct Max {
  T operator()(T a, T b) const { return std::max(a, b); }
};

// binary functions

class ConditionalFunction : public Function {
 public:
  ConditionalFunction() : Function("If", 3) {}

  virtual Token* CreateToken(Allocator& buffer,
                             Token** arguments,
                             int count) const override {
    assert(count == 3);
    return expression::CreateToken<TokenImpl>(buffer, arguments[0],
                                              arguments[1], arguments[2]);
  }

 private:
  class TokenImpl : public Token {
   public:
    TokenImpl(Token* condition, Token* when_true, Token* when_false)
        : condition_{condition},
          when_true_{when_true},
          when_false_{when_false} {}

    virtual Value Calculate(void* data) const override {
      auto cond = condition_->Calculate(data);

      Token* arg = cond ? when_true_ : when_false_;
      return arg->Calculate(data);
    }

    virtual void Traverse(TraverseCallback callb, void* param) const override {
      callb(this, param);
      callb(condition_, param);
      callb(when_true_, param);
      callb(when_false_, param);
    }

    virtual void Format(const FormatterDelegate& delegate,
                        std::string& str) const override {
      str += "If(";
      condition_->Format(delegate, str);
      str += ", ";
      when_true_->Format(delegate, str);
      str += ", ";
      when_false_->Format(delegate, str);
      str += ')';
    }

   private:
    Token* condition_;
    Token* when_true_;
    Token* when_false_;
  };
};

template <typename T>
class VariadicFunction : public Function {
 public:
  explicit VariadicFunction(std::string_view name) : Function{name, -1} {}

  virtual Token* CreateToken(Allocator& allocator,
                             Token** arguments,
                             int count) const override {
    // TODO: Create binary token.
    return expression::CreateToken<TokenImpl>(allocator, this, arguments, count,
                                              allocator);
  }

 private:
  class TokenImpl : public Token {
   public:
    TokenImpl(const VariadicFunction* fun,
              Token** params,
              int count,
              Allocator& allocator)
        : fun_{fun},
          params_{
              static_cast<Token**>(allocator.allocate(count * sizeof(Token*)))},
          count_{count} {
      std::copy(params, params + count, params_);
    }

    virtual Value Calculate(void* data) const override {
      auto val = params_[0]->Calculate(data);
      for (int i = 1; i < count_; i++) {
        auto pval = params_[i]->Calculate(data);
        val = T()(val, pval);
      }
      return val;
    }

    virtual void Traverse(TraverseCallback callb, void* param) const {
      callb(this, param);
      for (int i = 0; i < count_; ++i)
        callb(params_[i], param);
    }

    virtual void Format(const FormatterDelegate& delegate,
                        std::string& str) const override {
      str += fun_->name;
      str += '(';
      if (count_ != 0) {
        params_[0]->Format(delegate, str);
        for (int i = 1; i < count_; ++i) {
          str += ", ";
          params_[i]->Format(delegate, str);
        }
      }
      str += ')';
    }

   private:
    const VariadicFunction* fun_;
    Token** params_;
    int count_;
  };
};

class MathFunction1 : public Function {
 public:
  typedef double (*fun_t)(double);

  MathFunction1(std::string_view name, fun_t fun)
      : Function{name, 1}, fun_(fun) {}

  virtual Token* CreateToken(Allocator& allocator,
                             Token** arguments,
                             int count) const {
    assert(count == 1);
    return expression::CreateToken<TokenImpl>(allocator, this, arguments[0]);
  }

 private:
  class TokenImpl : public Token {
   public:
    TokenImpl(const MathFunction1* fun, Token* argument)
        : fun_(fun), argument_{argument} {}

    virtual Value Calculate(void* data) const override {
      Value v = argument_->Calculate(data);
      return fun_->fun_(v);
    }

    virtual void Traverse(TraverseCallback callb, void* param) const {
      callb(this, param);
      callb(argument_, param);
    }

    virtual void Format(const FormatterDelegate& delegate,
                        std::string& str) const override {
      str += fun_->name;
      str += '(';
      argument_->Format(delegate, str);
      str += ')';
    }

   private:
    const MathFunction1* fun_;
    Token* argument_;
  };

  const fun_t fun_;
};

class MathFunction2 : public Function {
 public:
  typedef double (*fun_t)(double, double);

  MathFunction2(std::string_view name, fun_t fun)
      : Function{name, 2}, fun_{fun} {}

  virtual Token* CreateToken(Allocator& allocator,
                             Token** arguments,
                             int count) const {
    assert(count == 2);
    return expression::CreateToken<TokenImpl>(allocator, this, arguments[0],
                                              arguments[1]);
  }

 private:
  class TokenImpl : public Token {
   public:
    TokenImpl(const MathFunction2* fun, Token* left, Token* right)
        : fun_(fun), left_{left}, right_{right} {}

    virtual Value Calculate(void* data) const override {
      auto v1 = left_->Calculate(data);
      auto v2 = right_->Calculate(data);
      return fun_->fun_(v1, v2);
    }

    virtual void Traverse(TraverseCallback callb, void* param) const {
      callb(this, param);
      callb(left_, param);
      callb(right_, param);
    }

    virtual void Format(const FormatterDelegate& delegate,
                        std::string& str) const override {
      str += fun_->name;
      str += '(';
      left_->Format(delegate, str);
      str += ", ";
      right_->Format(delegate, str);
      str += ')';
    }

   private:
    const MathFunction2* fun_;
    Token* left_;
    Token* right_;
  };

  const fun_t fun_;
};

inline Function* FindFunction(Function** list, std::string_view name) {
  for (; *list; ++list) {
    if (EqualsNoCase((*list)->name, name))
      return *list;
  }
  return NULL;
}

Function* FindDefaultFunction(std::string_view name) {
  static VariadicFunction<std::logical_or<Value>> logical_or_fun("Or");
  static VariadicFunction<std::logical_and<Value>> logical_and_fun("And");
  static VariadicFunction<Min<Value>> min_fun("Min");
  static VariadicFunction<Max<Value>> max_fun("Max");
  static MathFunction1 abs_fun("Abs", abs_);
  static MathFunction1 not_fun("Not", not_);
  static MathFunction1 sign_fun("Sign", sign);
  static MathFunction1 sqrt_fun("Sqrt", sqrt);
  static MathFunction1 sin_fun("Sin", sin);
  static MathFunction1 cos_fun("Cos", cos);
  static MathFunction1 tan_fun("Tan", tan);
  static MathFunction1 asin_fun("ASin", asin);
  static MathFunction1 acos_fun("ACos", acos);
  static MathFunction1 atan_fun("ATan", atan);
  static MathFunction2 atan2_fun("ATan2", atan2);
  static MathFunction2 bitxor_fun("BitXor", xor_);
  static ConditionalFunction _if;

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

  return FindFunction(list, name);
}

}  // namespace functions

}  // namespace expression
