#pragma once

#include <math.h>
#include <string.h>
#include <stdexcept>

namespace expression {

class String {
 public:
  String() {}
  String(const char* data) { Set(data, strlen(data)); }
  String(const char* data, std::size_t size) { Set(data, size); }
  ~String() { delete[] data_; }

  String(const String& source) { Set(source.data_, source.size_); }

  String& operator=(const String& source) {
    if (&source != this)
      Set(source.data_, source.size_);
    return *this;
  }

 private:
  void Set(const char* data, std::size_t size) {
    delete[] data_;

    if (size != 0) {
      data_ = new char[size + 1];
      memcpy(data_, data, size);
      data_[size] = '\0';
    }

    size_ = size;
  }

  char* data_ = nullptr;
  std::size_t size_ = 0;
};

class Value {
 public:
  enum class Type { Number, String };

  Type type;

#pragma warning(push, 3)
  union {
    double number;
    struct {
      char* string;
      int length;
    } str;
  };
#pragma warning(pop)

  static double kPrecision;

  Value() : type(Type::Number), number(0.0) {}
  Value(double value) : type(Type::Number), number(value) {}
  Value(float value) : type(Type::Number), number(value) {}
  Value(int value) : type(Type::Number), number(value) {}
  Value(const char* str) : type(Type::String) {
    _set_string(str, (int)strlen(str));
  }
  Value(const char* str, int length) : type(Type::String) {
    _set_string(str, length);
  }
  Value(std::string_view str) : type(Type::String) {
    _set_string(str.data(), str.size());
  }
  Value(const std::string& str) : type(Type::String) {
    _set_string(str.data(), str.size());
  }
  Value(const Value& right) { _set(right); }

  ~Value() { _clear(); }

  bool is_number() const { return type == Type::Number; }
  bool is_string() const { return type == Type::String; }

  void _set_string(const char* str, int length) {
    this->str.length = length;
    this->str.string = new char[length + 1];
    memcpy(this->str.string, str, length);
    this->str.string[length] = '\0';
  }

  void set_string(const char* str, int length) {
    _clear();
    _set_string(str, (int)strlen(str));
    type = Type::String;
  }

  void _set(const Value& right) {
    type = right.type;
    switch (type) {
      case Type::Number:
        number = right.number;
        break;
      case Type::String:
        _set_string(right.str.string, right.str.length);
        break;
      default:
        _bad_type();
        break;
    }
  }

  void _clear() {
    if (type == Type::String)
      delete[] str.string;
  }

  static void _bad_type() { throw std::runtime_error("bad type"); }

  void swap(Value& value) {
    char buf[sizeof(Value)];
    memcpy(&buf, this, sizeof(Value));
    memcpy(this, &value, sizeof(Value));
    memcpy(&value, buf, sizeof(Value));
  }

  operator int() const { return (int)(double)number; }
  operator float() const { return (float)(double)*this; }
  operator double() const {
    if (type != Type::Number)
      _bad_type();
    return number;
  }
  operator bool() const { return (double)*this >= kPrecision; }
  operator const char*() const {
    if (type != Type::String)
      _bad_type();
    return str.string;
  }
  operator double&() {
    if (type != Type::Number)
      _bad_type();
    return number;
  }

  Value& operator=(double value) {
    _clear();
    type = Type::Number;
    number = value;
    return *this;
  }

  Value& operator=(const Value& right) {
    _clear();
    _set(right);
    return *this;
  }

  Value& operator=(const char* str) {
    set_string(str, (int)strlen(str));
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
    if (type != right.type)
      _bad_type();
    if (type == Type::Number) {
      static_cast<double&>(*this) += static_cast<double>(right);
    } else {
      auto s = std::string(str.string, str.length) +
               std::string(right.str.string, right.str.length);
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
    return type == Type::Number && fabs(number - value) < kPrecision;
  }
  bool operator==(int value) const { return *this == (double)value; }

  bool operator==(const Value& right) const {
    if (type != right.type)
      return false;
    switch (type) {
      case Type::Number:
        return fabs(number - right.number) < kPrecision;
      case Type::String:
        return strcmp(str.string, right.str.string) == 0;
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
    switch (type) {
      case Type::Number:
        return number < (double)right;
      case Type::String:
        return strcmp(str.string, (const char*)right) < 0;
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
};

}  // namespace expression
