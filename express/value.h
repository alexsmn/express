#pragma once

#include <math.h>
#include <string.h>
#include <stdexcept>

class Value
{
public:
  enum { NUMBER, STRING };

  int				type;

#pragma warning(push,3)
  union {
    double		number;
    struct {
      char*	string;
      int		length;
    };
  };
#pragma warning(pop)

  static double kPrecision;

  Value() : type(NUMBER), number(0.0) { }
  Value(double value) : type(NUMBER), number(value) { }
  Value(float value) : type(NUMBER), number(value) { }
  Value(int value) : type(NUMBER), number(value) { }
  Value(const char* str) : type(STRING) { _set_string(str, (int)strlen(str)); }
  Value(const char* str, int length) : type(STRING) { _set_string(str, length); }
  Value(const Value& right) { _set(right); }
  
  ~Value() { _clear(); }

  bool is_number() const { return type == NUMBER; }
  bool is_string() const { return type == STRING; }

  void _set_string(const char* str, int length)
  {
    this->length = length;
    this->string = new char[length + 1];
    memcpy(this->string, str, length + 1);
  }

  void set_string(const char* str, int length)
  {
    _clear();
    _set_string(str, (int)strlen(str));
    type = STRING;
  }

  void _set(const Value& right)
  {
    type = right.type;
    switch (type)
    {
    case NUMBER:
      number = right.number;
      break;
    case STRING:
      _set_string(right.string, right.length);
      break;
    default:
      _bad_type();
      break;
    }
  }

  void _clear()
  {
    if (type == STRING)
      delete[] string;
  }

  static void _bad_type()
  {
    throw std::runtime_error("bad type");
  }

  void swap(Value& value)
  {
    char buf[sizeof(Value)];
    memcpy(&buf, this, sizeof(Value));
    memcpy(this, &value, sizeof(Value));
    memcpy(&value, buf, sizeof(Value));
  }

  operator int() const { return (int)(double)number; }
  operator float() const { return (float)(double)*this; }
  operator double() const { if (type != NUMBER) _bad_type(); return number; }
  operator bool() const { return (double)*this >= kPrecision; }
  operator const char*() const { if (type != STRING) _bad_type(); return string; }
  operator double&() { if (type != NUMBER) _bad_type(); return number; }

  Value& operator =(double value)
  {
    _clear();
    type = NUMBER;
    number = value;
    return *this;
  }

  Value& operator =(const Value& right)
  {
    _clear();
    _set(right);
    return *this;
  }

  Value& operator =(const char* str)
  {
    set_string(str, (int)strlen(str));
    return *this;
  }

  /*Value operator +(const Value& right) const { return Value((double)*this + (double)right); }
  Value operator -(const Value& right) const { return Value((double)*this - (double)right); }
  Value operator *(const Value& right) const { return Value((double)*this * (double)right); }
  Value operator /(const Value& right) const { return Value((double)*this / (double)right); }*/

/*	Value& operator +=(const Value& right) { (double)*this += (double)right; return *this; }
  Value& operator -=(const Value& right) { (double)*this -= (double)right; return *this; }
  Value& operator *=(const Value& right) { (double)*this *= (double)right; return *this; }
  Value& operator /=(const Value& right) { (double)*this /= (double)right; return *this; }*/

  double operator +(double right) const { return static_cast<double>(*this) + right; }
  double operator -(double right) const { return static_cast<double>(*this) - right; }
  double operator *(double right) const { return static_cast<double>(*this) * right; }
  double operator /(double right) const { return static_cast<double>(*this) / right; }

  Value& operator +=(double right) { static_cast<double&>(*this) += right; return *this; }
  Value& operator -=(double right) { static_cast<double&>(*this) -= right; return *this; }
  Value& operator *=(double right) { static_cast<double&>(*this) *= right; return *this; }
  Value& operator /=(double right) { static_cast<double&>(*this) /= right; return *this; }

  bool operator ==(double value) const { return type == NUMBER && fabs(number - value) < kPrecision; }
  bool operator ==(int value) const { return *this == (double)value; }

  bool operator ==(const Value& right) const
  {
    if (type != right.type)
      return false;
    switch (type)
    {
    case NUMBER:
      return fabs(number - right.number) < kPrecision;
    case STRING:
      return strcmp(string, right.string) == 0;
    default:
      _bad_type();
      return false;
    }
  }

  bool operator !=(const Value& right) const { return !operator ==(right); }
  template<typename T> bool operator !=(T right) const { return !operator ==(right); }

  bool operator <(const Value& right) const
  {
    switch (type)
    {
    case NUMBER:
      return number < (double)right;
    case STRING:
      return strcmp(string, (const char*)right) < 0;
    default:
      _bad_type();
      return false;
    }
  }

  bool operator >(const Value& right) const
  {
    switch (type)
    {
    case NUMBER:
      return number > (double)right;
    case STRING:
      return strcmp(string, (const char*)right) > 0;
    default:
      _bad_type();
      return false;
    }
  }

  bool operator <=(const Value& right) const
  {
    switch (type)
    {
    case NUMBER:
      return number < right.number || fabs(number - right.number) < kPrecision;
    case STRING:
      return strcmp(string, right.string) <= 0;
    default:
      _bad_type();
      return false;
    }
  }

  bool operator >=(const Value& right) const
  {
    switch (type)
    {
    case NUMBER:
      return number > right.number || fabs(number - right.number) < kPrecision;
    case STRING:
      return strcmp(string, right.string) >= 0;
    default:
      _bad_type();
      return false;
    }
  }

  Value operator -() const { return -(double)*this; }
  bool operator !() const { return !(bool)*this; }
};
