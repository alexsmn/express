#pragma once

#include "express/allocator.h"
#include "express/value.h"

namespace expression {

class FormatterDelegate;

template <class BasicToken>
using BasicTraverseCallback = bool (*)(const BasicToken* token, void* param);

template <class BasicToken>
class BasicExpression {
 public:
  using BasicValue =
      decltype(std::declval<typename BasicToken>().Calculate(nullptr));

  BasicExpression() {}
  ~BasicExpression() { Clear(); }

  BasicExpression(const BasicExpression&) = delete;
  BasicExpression& operator=(const BasicExpression&) = delete;

  void swap(BasicExpression& other) {
    allocator_.swap(other.allocator_);
    std::swap(root_, other.root_);
  }

  template <class Parser>
  void Parse(Parser& parser, Allocator& allocator);

  BasicValue Calculate(void* data = NULL) const;

  void Traverse(BasicTraverseCallback<BasicToken> callback, void* param) const;

  std::string Format(const FormatterDelegate& delegate) const;

  void Clear();

 protected:
  Allocator allocator_;
  BasicToken* root_ = nullptr;
};

template <class BasicToken>
template <class Parser>
inline void BasicExpression<BasicToken>::Parse(Parser& parser,
                                               Allocator& allocator) {
  auto* root = parser.Parse();
  if (!root)
    throw std::runtime_error("expression expected");

  allocator_ = std::move(allocator);
  root_ = root;
}

template <class BasicToken>
inline typename BasicExpression<BasicToken>::BasicValue
BasicExpression<BasicToken>::Calculate(void* data) const {
  assert(root_);
  return root_->Calculate(data);
}

template <class BasicToken>
inline void BasicExpression<BasicToken>::Traverse(
    BasicTraverseCallback<BasicToken> callback,
    void* param) const {
  assert(root_);
  root_->Traverse(callback, param);
}

template <class BasicToken>
inline std::string BasicExpression<BasicToken>::Format(
    const FormatterDelegate& delegate) const {
  assert(root_);
  std::string str;
  root_->Format(delegate, str);
  return str;
}

template <class BasicToken>
inline void BasicExpression<BasicToken>::Clear() {
  allocator_.clear();
  root_ = nullptr;
}

}  // namespace expression
