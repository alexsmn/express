#pragma once

#include <limits>
#include <math.h>
#include <stdexcept>
#include <string.h>
#include <string_view>

namespace expression {

class Value {
 public:
  enum class Type { Number, String };

  static constexpr double kPrecision = std::numeric_limits<double>::epsilon();
  static constexpr int kInlineStringCapacity = 23;

  Value() noexcept : type_(Type::Number), number_(0.0) {}
  Value(double value) noexcept : type_(Type::Number), number_(value) {}
  Value(float value) noexcept : type_(Type::Number), number_(value) {}
  Value(int value) noexcept : type_(Type::Number), number_(value) {}
  Value(const char* string) : Value(string, static_cast<int>(strlen(string))) {}
  Value(const char* string, int length) {
    _set_string(std::string_view(string, static_cast<size_t>(length)));
  }
  Value(std::string_view string)
      : Value(string.data(), static_cast<int>(string.size())) {}
  Value(const std::string& string)
      : Value(string.data(), static_cast<int>(string.size())) {}
  Value(const Value& right) { _set(right); }
  Value(Value&& right) noexcept { _move_from(std::move(right)); }

  ~Value() { _clear(); }

  Value& operator=(Value&& right) noexcept {
    if (this == &right)
      return *this;
    _clear();
    _move_from(std::move(right));
    return *this;
  }

  constexpr Type type() const noexcept { return type_; }

  bool is_number() const noexcept { return type_ == Type::Number; }
  bool is_string() const noexcept { return type_ == Type::String; }

  void set_string(const char* string, int length) {
    _clear();
    _set_string(std::string_view(string, static_cast<size_t>(length)));
  }

  void swap(Value& value) {
    Value tmp(std::move(value));
    value = std::move(*this);
    *this = std::move(tmp);
  }

  operator int() const { return static_cast<int>(static_cast<double>(*this)); }
  operator float() const { return static_cast<float>(static_cast<double>(*this)); }
  operator double() const {
    if (type_ != Type::Number)
      _bad_type();
    return number_;
  }
  operator bool() const { return fabs((double)*this) >= kPrecision; }
  operator const char*() const {
    if (type_ != Type::String)
      _bad_type();
    return string_data();
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
    if (this == &right)
      return *this;
    _clear();
    _set(right);
    return *this;
  }

  Value& operator=(const char* string) {
    set_string(string, static_cast<int>(strlen(string)));
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
      append_string(right.string_view());
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
        return string_length_ == right.string_length_ &&
               memcmp(string_data(), right.string_data(),
                      static_cast<size_t>(string_length_)) == 0;
      default:
        _bad_type();
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
      case Type::String: {
        const int compare = memcmp(
            string_data(), right.string_data(),
            static_cast<size_t>(std::min(string_length_, right.string_length_)));
        if (compare != 0)
          return compare < 0;
        return string_length_ < right.string_length_;
      }
      default:
        _bad_type();
    }
  }

  bool operator>(const Value& right) const { return right < *this; }

  bool operator<=(const Value& right) const { return !(right < *this); }

  bool operator>=(const Value& right) const { return !(*this < right); }

  Value operator-() const { return -(double)*this; }
  bool operator!() const { return !(bool)*this; }

 private:
  std::string_view string_view() const noexcept {
    return std::string_view(string_data(), static_cast<size_t>(string_length_));
  }

  const char* string_data() const noexcept {
    return string_is_inline_ ? inline_string_ : heap_string_;
  }

  char* mutable_string_data() noexcept {
    return string_is_inline_ ? inline_string_ : heap_string_;
  }

  bool can_store_inline(int length) const noexcept {
    return length <= kInlineStringCapacity;
  }

  void _set_string(std::string_view string) {
    type_ = Type::String;
    string_length_ = static_cast<int>(string.size());
    string_is_inline_ = can_store_inline(string_length_);

    char* dest = nullptr;
    if (string_is_inline_) {
      dest = inline_string_;
    } else {
      heap_string_ = new char[string.size() + 1];
      dest = heap_string_;
    }

    memcpy(dest, string.data(), string.size());
    dest[string.size()] = '\0';
  }

  void _set(const Value& right) {
    type_ = right.type_;
    switch (type_) {
      case Type::Number:
        number_ = right.number_;
        string_length_ = 0;
        string_is_inline_ = true;
        break;
      case Type::String:
        _set_string(right.string_view());
        break;
      default:
        _bad_type();
    }
  }

  void _move_from(Value&& right) noexcept {
    type_ = right.type_;
    switch (type_) {
      case Type::Number:
        number_ = right.number_;
        string_length_ = 0;
        string_is_inline_ = true;
        break;
      case Type::String:
        string_length_ = right.string_length_;
        string_is_inline_ = right.string_is_inline_;
        if (string_is_inline_) {
          memcpy(inline_string_, right.inline_string_,
                 static_cast<size_t>(string_length_) + 1);
        } else {
          heap_string_ = right.heap_string_;
          right.heap_string_ = nullptr;
        }
        right.type_ = Type::Number;
        right.number_ = 0.0;
        right.string_length_ = 0;
        right.string_is_inline_ = true;
        break;
      default:
        _bad_type();
    }
  }

  void _clear() {
    if (type_ == Type::String && !string_is_inline_)
      delete[] heap_string_;
    type_ = Type::Number;
    number_ = 0.0;
    string_length_ = 0;
    string_is_inline_ = true;
  }

  void append_string(std::string_view right) {
    const int new_length = string_length_ + static_cast<int>(right.size());
    if (can_store_inline(new_length)) {
      char* dest = inline_string_;
      if (!string_is_inline_) {
        memcpy(dest, heap_string_, static_cast<size_t>(string_length_));
        delete[] heap_string_;
      }
      memcpy(dest + string_length_, right.data(), right.size());
      dest[new_length] = '\0';
      string_length_ = new_length;
      string_is_inline_ = true;
      return;
    }

    char* new_string = new char[static_cast<size_t>(new_length) + 1];
    memcpy(new_string, string_data(), static_cast<size_t>(string_length_));
    memcpy(new_string + string_length_, right.data(), right.size());
    new_string[new_length] = '\0';
    if (!string_is_inline_)
      delete[] heap_string_;
    heap_string_ = new_string;
    string_length_ = new_length;
    string_is_inline_ = false;
  }

  [[noreturn]] static void _bad_type() {
    throw std::runtime_error("bad type_");
  }

  Type type_ = Type::Number;
  int string_length_ = 0;
  bool string_is_inline_ = true;

#pragma warning(push, 3)
  union {
    double number_;
    char* heap_string_;
    char inline_string_[kInlineStringCapacity + 1];
  };
#pragma warning(pop)
};

}  // namespace expression
