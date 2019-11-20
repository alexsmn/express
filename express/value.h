#pragma once

#include <math.h>
#include <string.h>
#include <stdexcept>

namespace expression {

class Value {
 public:
  enum class Type { Number, String };

  constexpr Type type() const noexcept { return type_; }

  static constexpr double kPrecision = std::numeric_limits<double>::epsilon();

  Value() : type_(Type::Number), number_(0.0) {}
  Value(double value) : type_(Type::Number), number_(value) {}
  Value(float value) : type_(Type::Number), number_(value) {}
  Value(int value) : type_(Type::Number), number_(value) {}
  Value(const char* string) : type_(Type::String) {
    _set_string(string, (int)strlen(string));
  }
  Value(const char* string, int length) : type_(Type::String) {
    _set_string(string, length);
  }
  Value(std::string_view string) : type_(Type::String) {
    _set_string(string.data(), string.size());
  }
  Value(const std::string& string) : type_(Type::String) {
    _set_string(string.data(), string.size());
  }
  Value(const Value& right) { _set(right); }

  ~Value() { _clear(); }

  bool is_number() const { return type_ == Type::Number; }
  bool is_string() const { return type_ == Type::String; }

  void _set_string(const char* string, int length) {
    string_.length = length;
    string_.string = new char[length + 1];
    memcpy(string_.string, string, length);
    string_.string[length] = '\0';
  }

  void set_string(const char* string, int length) {
    _clear();
    _set_string(string, (int)strlen(string));
    type_ = Type::String;
  }

  void _set(const Value& right) {
    type_ = right.type_;
    switch (type_) {
      case Type::Number:
        number_ = right.number_;
        break;
      case Type::String:
        _set_string(right.string_.string, right.string_.length);
        break;
      default:
        _bad_type();
        break;
    }
  }

  void _clear() {
    if (type_ == Type::String)
      delete[] string_.string;
  }

  static void _bad_type() { throw std::runtime_error("bad type_"); }

  void swap(Value& value) {
    char buf[sizeof(Value)];
    memcpy(&buf, this, sizeof(Value));
    memcpy(this, &value, sizeof(Value));
    memcpy(&value, buf, sizeof(Value));
  }

  operator int() const { return (int)(double)number_; }
  operator float() const { return (float)(double)*this; }
  operator double() const {
    if (type_ != Type::Number)
      _bad_type();
    return number_;
  }
  operator bool() const { return (double)*this >= kPrecision; }
  operator const char*() const {
    if (type_ != Type::String)
      _bad_type();
    return string_.string;
  }
  operator double&() {
    if (type_ != Type::Number)
      _bad_type();
    return number_;
  }

  Value& operator=(double value) {
    _clear();
    type_ = Type::Number;
    number_ = value;
    return *this;
  }

  Value& operator=(const Value& right) {
    _clear();
    _set(right);
    return *this;
  }

  Value& operator=(const char* string) {
    set_string(string, (int)strlen(string));
    return *this;
  }

  double operator+(double right) const {
    return static_cast<double>(*this) + right;
  }
  double operator-(double right) const {
    return static_cast<double>(*this) - right;
  }
  double operator*(double right) const {
    return static_cast<double>(*this) * right;
  }
  double operator/(double right) const {
    return static_cast<double>(*this) / right;
  }

  Value& operator+=(const Value& right) {
    if (type_ != right.type_)
      _bad_type();
    if (type_ == Type::Number) {
      static_cast<double&>(*this) += static_cast<double>(right);
    } else {
      auto s = std::string(string_.string, string_.length) +
               std::string(right.string_.string, right.string_.length);
      set_string(s.data(), s.size());
    }
    return *this;
  }
  Value& operator-=(double right) {
    static_cast<double&>(*this) -= right;
    return *this;
  }
  Value& operator*=(double right) {
    static_cast<double&>(*this) *= right;
    return *this;
  }
  Value& operator/=(double right) {
    static_cast<double&>(*this) /= right;
    return *this;
  }

  bool operator==(double value) const {
    return type_ == Type::Number && fabs(number_ - value) < kPrecision;
  }
  bool operator==(int value) const { return *this == (double)value; }

  bool operator==(const Value& right) const {
    if (type_ != right.type_)
      return false;
    switch (type_) {
      case Type::Number:
        return fabs(number_ - right.number_) < kPrecision;
      case Type::String:
        return strcmp(string_.string, right.string_.string) == 0;
      default:
        _bad_type();
        return false;
    }
  }

  bool operator!=(const Value& right) const { return !operator==(right); }
  template <typename T>
  bool operator!=(T right) const {
    return !operator==(right);
  }

  bool operator<(const Value& right) const {
    switch (type_) {
      case Type::Number:
        return number_ < (double)right;
      case Type::String:
        return strcmp(string_.string, (const char*)right) < 0;
      default:
        _bad_type();
        return false;
    }
  }

  bool operator>(const Value& right) const { return right < *this; }

  bool operator<=(const Value& right) const { return !(right < *this); }

  bool operator>=(const Value& right) const { return !(*this < right); }

  Value operator-() const { return -(double)*this; }
  bool operator!() const { return !(bool)*this; }

 private:
  Type type_;

#pragma warning(push, 3)
  union {
    double number_;
    struct {
      char* string;
      int length;
    } string_;
  };
#pragma warning(pop)
};

}  // namespace expression
