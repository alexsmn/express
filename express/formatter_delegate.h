#pragma once

#include "express/express_export.h"

#include <string>

namespace expression {

class EXPRESS_EXPORT FormatterDelegate {
 public:
  virtual ~FormatterDelegate() {}

  virtual void AppendDouble(std::string& str, double value) const {
    str.append(std::to_string(value));
  }
};

}  // namespace expression