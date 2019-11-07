#pragma once

#include "express/express_export.h"
#include "express/value.h"

#include <string>

namespace expression {

class EXPRESS_EXPORT FormatterDelegate {
 public:
  virtual ~FormatterDelegate() {}

  virtual void AppendDouble(std::string& str, double value) const {
    str.append(std::to_string(value));
  }

  virtual void AppendString(std::string& str, std::string_view s) const {
    str.reserve(str.size() + s.size() + 2);
    str += '"';
    str += s;
    str += '"';
  }
};

inline void AppendValue(std::string& str,
                        const Value& value,
                        const FormatterDelegate& delegate) {
  switch (value.type) {
    case Value::Type::String:
      delegate.AppendString(str, static_cast<std::string_view>(value));
      break;
    case Value::Type::Number:
      delegate.AppendDouble(str, static_cast<double>(value));
      break;
    default:
      assert(false);
      break;
  }
}

}  // namespace expression