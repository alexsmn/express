#pragma once

#include "express/express.h"
#include "express/function.h"
#include "express/strings.h"

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

inline double sign(double x) {
  if (value_is_null(x))
    return 0.0;
  else if (x > 0.0)
    return 1.0;
  else
    return -1.0;
}

inline double abs_(double x) {
  return abs(x);
}

inline double not_(double x) {
  return bool_to_value(!value_to_bool(x));
}

inline double xor_(double x, double y) {
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

template <class BasicToken>
class BasicConditionalFunction : public BasicFunction<BasicToken> {
 public:
  BasicConditionalFunction() : BasicFunction<BasicToken>("If", 3) {}

  virtual BasicToken MakeToken(Allocator& allocator,
                               BasicToken* arguments,
                               size_t argument_count) const override {
    assert(argument_count == 3);
    Token* token = CreateToken<TokenImpl>(allocator, std::move(arguments[0]),
                                          std::move(arguments[1]),
                                          std::move(arguments[2]));
    return BasicToken{token};
  }

 private:
  class TokenImpl : public Token {
   public:
    TokenImpl(BasicToken&& condition,
              BasicToken&& when_true,
              BasicToken&& when_false)
        : condition_{std::move(condition)},
          when_true_{std::move(when_true)},
          when_false_{std::move(when_false)} {}

    virtual Value Calculate(void* data) const override {
      auto condition_value = condition_.Calculate(data);
      const BasicToken& arg = condition_value ? when_true_ : when_false_;
      return arg.Calculate(data);
    }

    virtual void Traverse(TraverseCallback callback,
                          void* param) const override {
      callback(this, param);
      condition_.Traverse(callback, param);
      when_true_.Traverse(callback, param);
      when_false_.Traverse(callback, param);
    }

    virtual void Format(const FormatterDelegate& delegate,
                        std::string& str) const override {
      str += "If(";
      condition_.Format(delegate, str);
      str += ", ";
      when_true_.Format(delegate, str);
      str += ", ";
      when_false_.Format(delegate, str);
      str += ')';
    }

   private:
    const BasicToken condition_;
    const BasicToken when_true_;
    const BasicToken when_false_;
  };
};

template <typename BasicToken, typename T>
class BasicVariadicFunction : public BasicFunction<BasicToken> {
 public:
  explicit BasicVariadicFunction(std::string_view name)
      : BasicFunction<BasicToken>{name, -1} {}

  virtual BasicToken MakeToken(Allocator& allocator,
                               BasicToken* arguments,
                               size_t argument_count) const override {
    // TODO: Create binary token.
    Token* token = CreateToken<TokenImpl>(allocator, *this, arguments,
                                          argument_count, allocator);
    return BasicToken{token};
  }

 private:
  class TokenImpl : public Token {
   public:
    TokenImpl(const BasicVariadicFunction& fun,
              BasicToken* arguments,
              size_t argument_count,
              Allocator& allocator)
        : fun_{fun},
          params_{static_cast<BasicToken*>(
              allocator.allocate(argument_count * sizeof(BasicToken)))},
          count_{argument_count} {
      std::move(arguments, arguments + argument_count, params_);
    }

    virtual Value Calculate(void* data) const override {
      assert(count_ >= 1);
      auto val = params_[0].Calculate(data);
      for (size_t i = 1; i < count_; i++) {
        auto pval = params_[i].Calculate(data);
        val = T()(val, pval);
      }
      return val;
    }

    virtual void Traverse(TraverseCallback callback, void* param) const {
      callback(this, param);
      for (size_t i = 0; i < count_; ++i)
        params_[i].Traverse(callback, param);
    }

    virtual void Format(const FormatterDelegate& delegate,
                        std::string& str) const override {
      str += fun_.name;
      str += '(';
      if (count_ != 0) {
        params_[0].Format(delegate, str);
        for (size_t i = 1; i < count_; ++i) {
          str += ", ";
          params_[i].Format(delegate, str);
        }
      }
      str += ')';
    }

   private:
    const BasicVariadicFunction& fun_;
    BasicToken* params_;
    const size_t count_;
  };
};

template <class BasicToken>
class BasicMathFunction1 : public BasicFunction<BasicToken> {
 public:
  typedef double (*fun_t)(double);

  BasicMathFunction1(std::string_view name, fun_t fun)
      : BasicFunction<BasicToken>{name, 1}, fun_(fun) {}

  virtual BasicToken MakeToken(Allocator& allocator,
                               BasicToken* arguments,
                               size_t argument_count) const override {
    assert(argument_count == 1);
    Token* token =
        CreateToken<TokenImpl>(allocator, *this, std::move(arguments[0]));
    return BasicToken{token};
  }

 private:
  class TokenImpl : public Token {
   public:
    TokenImpl(const BasicMathFunction1& fun, BasicToken&& argument)
        : fun_(fun), argument_{std::move(argument)} {}

    virtual Value Calculate(void* data) const override {
      Value v = argument_.Calculate(data);
      return fun_.fun_(v);
    }

    virtual void Traverse(TraverseCallback callback, void* param) const {
      callback(this, param);
      argument_.Traverse(callback, param);
    }

    virtual void Format(const FormatterDelegate& delegate,
                        std::string& str) const override {
      str += fun_.name;
      str += '(';
      argument_.Format(delegate, str);
      str += ')';
    }

   private:
    const BasicMathFunction1& fun_;
    const BasicToken argument_;
  };

  const fun_t fun_;
};

template <class BasicToken>
class BasicMathFunction2 : public BasicFunction<BasicToken> {
 public:
  typedef double (*fun_t)(double, double);

  BasicMathFunction2(std::string_view name, fun_t fun)
      : BasicFunction<BasicToken>{name, 2}, fun_{fun} {}

  virtual BasicToken MakeToken(Allocator& allocator,
                               BasicToken* arguments,
                               size_t argument_count) const override {
    assert(argument_count == 2);
    Token* token = CreateToken<TokenImpl>(
        allocator, *this, std::move(arguments[0]), std::move(arguments[1]));
    return BasicToken{token};
  }

 private:
  class TokenImpl : public Token {
   public:
    TokenImpl(const BasicMathFunction2& fun,
              BasicToken&& left,
              BasicToken&& right)
        : fun_{fun}, left_{std::move(left)}, right_{std::move(right)} {}

    virtual Value Calculate(void* data) const override {
      auto v1 = left_.Calculate(data);
      auto v2 = right_.Calculate(data);
      return fun_.fun_(v1, v2);
    }

    virtual void Traverse(TraverseCallback callback, void* param) const {
      callback(this, param);
      left_.Traverse(callback, param);
      right_.Traverse(callback, param);
    }

    virtual void Format(const FormatterDelegate& delegate,
                        std::string& str) const override {
      str += fun_.name;
      str += '(';
      left_.Format(delegate, str);
      str += ", ";
      right_.Format(delegate, str);
      str += ')';
    }

   private:
    const BasicMathFunction2& fun_;
    const BasicToken left_;
    const BasicToken right_;
  };

  const fun_t fun_;
};

template <class F>
inline const F* FindFunction(const F** list, std::string_view name) {
  for (; *list; ++list) {
    if (EqualsNoCase((*list)->name, name))
      return *list;
  }
  return NULL;
}

template <class BasicToken>
inline const BasicFunction<BasicToken>* FindDefaultFunction(
    std::string_view name) {
  static BasicVariadicFunction<BasicToken, std::logical_or<Value>>
      logical_or_fun("Or");
  static BasicVariadicFunction<BasicToken, std::logical_and<Value>>
      logical_and_fun("And");
  static BasicVariadicFunction<BasicToken, Min<Value>> min_fun("Min");
  static BasicVariadicFunction<BasicToken, Max<Value>> max_fun("Max");
  static BasicMathFunction1<BasicToken> abs_fun("Abs", abs_);
  static BasicMathFunction1<BasicToken> not_fun("Not", not_);
  static BasicMathFunction1<BasicToken> sign_fun("Sign", sign);
  static BasicMathFunction1<BasicToken> sqrt_fun("Sqrt", sqrt);
  static BasicMathFunction1<BasicToken> sin_fun("Sin", sin);
  static BasicMathFunction1<BasicToken> cos_fun("Cos", cos);
  static BasicMathFunction1<BasicToken> tan_fun("Tan", tan);
  static BasicMathFunction1<BasicToken> asin_fun("ASin", asin);
  static BasicMathFunction1<BasicToken> acos_fun("ACos", acos);
  static BasicMathFunction1<BasicToken> atan_fun("ATan", atan);
  static BasicMathFunction2<BasicToken> atan2_fun("ATan2", atan2);
  static BasicMathFunction2<BasicToken> bitxor_fun("BitXor", xor_);
  static BasicConditionalFunction<BasicToken> _if;

  static const BasicFunction<BasicToken>* list[] = {&logical_or_fun,
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
