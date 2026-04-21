#pragma once

#include "express/strings.h"
#include "express/token.h"

#include <cmath>
#include <optional>
#include <string_view>
#include <vector>

namespace expression {

template <class T>
class ValueToken : public Token {
 public:
  template <class U>
  explicit ValueToken(U&& value) : value_{std::forward<U>(value)} {}

  virtual Value Calculate(void* data) const override { return value_; }

  virtual void Traverse(TraverseCallback callback, void* param) const {
    callback(this, param);
  }

  virtual void Format(const FormatterDelegate& delegate,
                      std::string& str) const override {
    delegate.AppendDouble(str, value_);
  }

 private:
  const T value_;
};

class StringValueToken : public Token {
 public:
  StringValueToken(std::string_view str, Allocator& allocator)
      : str_{AllocateLiteralStorage(str, allocator)} {}

  virtual Value Calculate(void* data) const override {
    return Value{str_};
  }

  virtual void Traverse(TraverseCallback callback, void* param) const override {
    callback(this, param);
  }

  virtual void Format(const FormatterDelegate& delegate,
                      std::string& str) const override {
    str += '"';
    str.append(str_.data(), str_.size());
    str += '"';
  }

 private:
  static std::string_view AllocateLiteralStorage(std::string_view str,
                                                 Allocator& allocator) {
    auto* storage =
        static_cast<char*>(allocator.allocate(str.size() + 1, alignof(char)));
    memcpy(storage, str.data(), str.size());
    storage[str.size()] = '\0';
    return std::string_view(storage, str.size());
  }
  const std::string_view str_;
};

template <class OperandToken>
class BasicUnaryOperatorToken : public Token {
 public:
  template <class U>
  BasicUnaryOperatorToken(char oper, U&& operand)
      : operator_{oper}, operand_{std::forward<U>(operand)} {}

  virtual Value Calculate(void* data) const override {
    auto val = operand_.Calculate(data);
    switch (operator_) {
      case '-':
        val = -val;
        break;
      case '!':
        val = !val;
        break;
      default:
        assert(false);
        break;
    }
    return val;
  }

  virtual void Traverse(TraverseCallback callback, void* param) const override {
    callback(this, param);
    operand_.Traverse(callback, param);
  }

  virtual void Format(const FormatterDelegate& delegate,
                      std::string& str) const override {
    str += operator_;
    operand_.Format(delegate, str);
  }

 private:
  const char operator_;
  const OperandToken operand_;
};

template <class OperandToken>
class BasicBinaryOperatorToken : public Token {
 public:
  template <class L, class R>
  BasicBinaryOperatorToken(char oper, L&& left, R&& right)
      : operator_{oper},
        left_{std::forward<L>(left)},
        right_{std::forward<R>(right)} {}

  virtual Value Calculate(void* data) const override {
    auto val = left_.Calculate(data);
    auto rval = right_.Calculate(data);

    switch (operator_) {
      case '+':
        val += rval;
        break;
      case '-':
        val -= rval;
        break;
      case '*':
        val *= rval;
        break;
      case '/':
        val /= rval;
        break;
      case '^':
        val = pow((double)val, (double)rval);
        break;
      case '=':
        val = val == rval;
        break;
      case '<':
        val = val < rval;
        break;
      case '>':
        val = val > rval;
        break;
      case 'l':
        val = val <= rval;
        break;
      case 'g':
        val = val >= rval;
        break;
      default:
        assert(false);
        break;
    }

    return val;
  }

  virtual void Traverse(TraverseCallback callback, void* param) const override {
    callback(this, param);
    left_.Traverse(callback, param);
    right_.Traverse(callback, param);
  }

  virtual void Format(const FormatterDelegate& delegate,
                      std::string& str) const override {
    left_.Format(delegate, str);
    str += ' ';
    switch (operator_) {
      case 'l':
        str += "<=";
        break;
      case 'g':
        str += ">=";
        break;
      default:
        str += operator_;
        break;
    }
    str += ' ';
    right_.Format(delegate, str);
  }

 private:
  const char operator_;
  const OperandToken left_;
  const OperandToken right_;
};

template <class NestedToken>
class ParenthesesToken : public Token {
 public:
  template <class U>
  explicit ParenthesesToken(U&& nested_token)
      : nested_token_{std::forward<U>(nested_token)} {}

  virtual Value Calculate(void* data) const override {
    return nested_token_.Calculate(data);
  }

  virtual void Traverse(TraverseCallback callback, void* param) const override {
    callback(this, param);
    nested_token_.Traverse(callback, param);
  }

  virtual void Format(const FormatterDelegate& delegate,
                      std::string& str) const override {
    str += '(';
    nested_token_.Format(delegate, str);
    str += ')';
  }

 private:
  const NestedToken nested_token_;
};

template <class T, class V>
inline Token* CreateValueToken(Allocator& allocator, V&& value) {
  return CreateToken<ValueToken<T>>(allocator, std::forward<V>(value));
}

template <class T, class V>
inline PolymorphicToken MakePolymorphicValueToken(Allocator& allocator,
                                                  V&& value) {
  return PolymorphicToken{
      *CreateValueToken<T>(allocator, std::forward<V>(value))};
}

namespace detail {

enum class FastNodeKind {
  Literal,
  UnaryOperator,
  BinaryOperator,
  Parentheses,
  FunctionUnary,
  FunctionBinary,
  Conditional,
};

enum class FastFunctionKind {
  Abs,
  Not,
  Sign,
  Sqrt,
  Sin,
  Cos,
  Tan,
  ASin,
  ACos,
  ATan,
  ATan2,
  BitXor,
  Min,
  Max,
  Or,
  And,
  If,
};

struct FastNode {
  FastNodeKind kind = FastNodeKind::Literal;
  Value literal;
  char oper = '\0';
  FastFunctionKind function = FastFunctionKind::Abs;
  size_t left = 0;
  size_t right = 0;
  size_t extra = 0;
};

class FastExpressionStorage;

class FastToken {
 public:
  FastToken() = default;
  FastToken(const FastExpressionStorage* storage, size_t node_index)
      : storage_{storage}, node_index_{node_index} {}

  Value Calculate(void* data) const;
  void Traverse(TraverseCallback callback, void* param) const;
  void Format(const FormatterDelegate& delegate, std::string& str) const;

  size_t node_index() const noexcept { return node_index_; }

 private:
  const FastExpressionStorage* storage_ = nullptr;
  size_t node_index_ = 0;
};

class FastExpressionStorage {
 public:
  using TokenType = FastToken;
  static constexpr size_t kInvalidNode = static_cast<size_t>(-1);

  FastToken MakeDoubleToken(double value) {
    FastNode node;
    node.kind = FastNodeKind::Literal;
    node.literal = value;
    return AppendNode(std::move(node));
  }

  FastToken MakeStringToken(std::string_view str) {
    FastNode node;
    node.kind = FastNodeKind::Literal;
    node.literal = str;
    return AppendNode(std::move(node));
  }

  template <class OperandToken>
  FastToken MakeUnaryOperatorToken(char oper, OperandToken&& operand_token) {
    FastNode node;
    node.kind = FastNodeKind::UnaryOperator;
    node.oper = oper;
    node.left = std::forward<OperandToken>(operand_token).node_index();
    return AppendNode(std::move(node));
  }

  template <class NestedToken>
  FastToken MakeParenthesesToken(NestedToken&& nested_token) {
    FastNode node;
    node.kind = FastNodeKind::Parentheses;
    node.left = std::forward<NestedToken>(nested_token).node_index();
    return AppendNode(std::move(node));
  }

  template <class LeftOperand, class RightOperand>
  FastToken MakeBinaryOperatorToken(char oper,
                                    LeftOperand&& left_operand,
                                    RightOperand&& right_operand) {
    FastNode node;
    node.kind = FastNodeKind::BinaryOperator;
    node.oper = oper;
    node.left = std::forward<LeftOperand>(left_operand).node_index();
    node.right = std::forward<RightOperand>(right_operand).node_index();
    return AppendNode(std::move(node));
  }

  FastToken MakeFunctionToken(std::string_view name,
                              std::vector<FastToken> arguments) {
    const auto function = FindFunction(name);
    if (!function.has_value())
      throw std::runtime_error{std::string{"function was not found: "} +
                               std::string{name}};

    if (*function == FastFunctionKind::ATan2 ||
        *function == FastFunctionKind::BitXor) {
      RequireArgumentCount(name, arguments, 2);
      return MakeBinaryFunction(*function, std::move(arguments[0]),
                                std::move(arguments[1]));
    }

    if (*function == FastFunctionKind::Abs || *function == FastFunctionKind::Not ||
        *function == FastFunctionKind::Sign ||
        *function == FastFunctionKind::Sqrt || *function == FastFunctionKind::Sin ||
        *function == FastFunctionKind::Cos || *function == FastFunctionKind::Tan ||
        *function == FastFunctionKind::ASin ||
        *function == FastFunctionKind::ACos ||
        *function == FastFunctionKind::ATan) {
      RequireArgumentCount(name, arguments, 1);
      return MakeUnaryFunction(*function, std::move(arguments[0]));
    }

    if (*function == FastFunctionKind::If) {
      RequireArgumentCount(name, arguments, 3);
      FastNode node;
      node.kind = FastNodeKind::Conditional;
      node.left = arguments[0].node_index();
      node.right = arguments[1].node_index();
      node.extra = arguments[2].node_index();
      return AppendNode(std::move(node));
    }

    if (arguments.empty())
      throw std::runtime_error{"no parameters provided"};

    return MakeFoldedFunction(*function, std::move(arguments));
  }

  Value Calculate(const FastToken& token, void* data) const {
    return CalculateNode(token.node_index(), data);
  }

  std::string Format(const FastToken& token,
                     const FormatterDelegate& delegate) const {
    std::string str;
    FormatNode(token.node_index(), delegate, str);
    return str;
  }

  void FormatNode(size_t node_index,
                  const FormatterDelegate& delegate,
                  std::string& str) const {
    const auto& node = nodes_[node_index];
    switch (node.kind) {
      case FastNodeKind::Literal:
        if (node.literal.is_number()) {
          delegate.AppendDouble(str, static_cast<double>(node.literal));
        } else {
          str += '"';
          str += static_cast<const char*>(node.literal);
          str += '"';
        }
        break;
      case FastNodeKind::UnaryOperator:
        str += node.oper;
        FormatNode(node.left, delegate, str);
        break;
      case FastNodeKind::BinaryOperator:
        FormatNode(node.left, delegate, str);
        str += ' ';
        switch (node.oper) {
          case 'l':
            str += "<=";
            break;
          case 'g':
            str += ">=";
            break;
          default:
            str += node.oper;
            break;
        }
        str += ' ';
        FormatNode(node.right, delegate, str);
        break;
      case FastNodeKind::Parentheses:
        str += '(';
        FormatNode(node.left, delegate, str);
        str += ')';
        break;
      case FastNodeKind::FunctionUnary:
        str += FunctionName(node.function);
        str += '(';
        FormatNode(node.left, delegate, str);
        str += ')';
        break;
      case FastNodeKind::FunctionBinary:
        str += FunctionName(node.function);
        str += '(';
        AppendFunctionArguments(node_index, delegate, str);
        str += ')';
        break;
      case FastNodeKind::Conditional:
        str += "If(";
        FormatNode(node.left, delegate, str);
        str += ", ";
        FormatNode(node.right, delegate, str);
        str += ", ";
        FormatNode(node.extra, delegate, str);
        str += ')';
        break;
    }
  }

  void Traverse(const FastToken& token,
                TraverseCallback callback,
                void* param) const {
    TraverseNode(token.node_index(), callback, param);
  }

  void TraverseNode(size_t node_index,
                    TraverseCallback callback,
                    void* param) const {
    class FastTokenView : public Token {
     public:
      FastTokenView(const FastExpressionStorage& storage, size_t node_index)
          : storage_{storage}, node_index_{node_index} {}

      Value Calculate(void* data) const override {
        return storage_.CalculateNode(node_index_, data);
      }

      void Traverse(TraverseCallback inner_callback, void* inner_param) const override {
        storage_.TraverseNode(node_index_, inner_callback, inner_param);
      }

      void Format(const FormatterDelegate& delegate,
                  std::string& str) const override {
        storage_.FormatNode(node_index_, delegate, str);
      }

     private:
      const FastExpressionStorage& storage_;
      size_t node_index_;
    } view(*this, node_index);

    if (!callback(&view, param))
      return;

    const auto& node = nodes_[node_index];
    switch (node.kind) {
      case FastNodeKind::Literal:
        break;
      case FastNodeKind::UnaryOperator:
      case FastNodeKind::Parentheses:
      case FastNodeKind::FunctionUnary:
        TraverseNode(node.left, callback, param);
        break;
      case FastNodeKind::BinaryOperator:
      case FastNodeKind::FunctionBinary:
        TraverseNode(node.left, callback, param);
        TraverseNode(node.right, callback, param);
        break;
      case FastNodeKind::Conditional:
        TraverseNode(node.left, callback, param);
        TraverseNode(node.right, callback, param);
        TraverseNode(node.extra, callback, param);
        break;
    }
  }

 private:
  static std::optional<FastFunctionKind> FindFunction(std::string_view name) {
    if (EqualsNoCase(name, "Abs"))
      return FastFunctionKind::Abs;
    if (EqualsNoCase(name, "Not"))
      return FastFunctionKind::Not;
    if (EqualsNoCase(name, "Sign"))
      return FastFunctionKind::Sign;
    if (EqualsNoCase(name, "Sqrt"))
      return FastFunctionKind::Sqrt;
    if (EqualsNoCase(name, "Sin"))
      return FastFunctionKind::Sin;
    if (EqualsNoCase(name, "Cos"))
      return FastFunctionKind::Cos;
    if (EqualsNoCase(name, "Tan"))
      return FastFunctionKind::Tan;
    if (EqualsNoCase(name, "ASin"))
      return FastFunctionKind::ASin;
    if (EqualsNoCase(name, "ACos"))
      return FastFunctionKind::ACos;
    if (EqualsNoCase(name, "ATan"))
      return FastFunctionKind::ATan;
    if (EqualsNoCase(name, "ATan2"))
      return FastFunctionKind::ATan2;
    if (EqualsNoCase(name, "BitXor"))
      return FastFunctionKind::BitXor;
    if (EqualsNoCase(name, "Min"))
      return FastFunctionKind::Min;
    if (EqualsNoCase(name, "Max"))
      return FastFunctionKind::Max;
    if (EqualsNoCase(name, "Or"))
      return FastFunctionKind::Or;
    if (EqualsNoCase(name, "And"))
      return FastFunctionKind::And;
    if (EqualsNoCase(name, "If"))
      return FastFunctionKind::If;
    return std::nullopt;
  }

  static std::string_view FunctionName(FastFunctionKind function) {
    switch (function) {
      case FastFunctionKind::Abs:
        return "Abs";
      case FastFunctionKind::Not:
        return "Not";
      case FastFunctionKind::Sign:
        return "Sign";
      case FastFunctionKind::Sqrt:
        return "Sqrt";
      case FastFunctionKind::Sin:
        return "Sin";
      case FastFunctionKind::Cos:
        return "Cos";
      case FastFunctionKind::Tan:
        return "Tan";
      case FastFunctionKind::ASin:
        return "ASin";
      case FastFunctionKind::ACos:
        return "ACos";
      case FastFunctionKind::ATan:
        return "ATan";
      case FastFunctionKind::ATan2:
        return "ATan2";
      case FastFunctionKind::BitXor:
        return "BitXor";
      case FastFunctionKind::Min:
        return "Min";
      case FastFunctionKind::Max:
        return "Max";
      case FastFunctionKind::Or:
        return "Or";
      case FastFunctionKind::And:
        return "And";
      case FastFunctionKind::If:
        return "If";
    }

    return {};
  }

  static void RequireArgumentCount(std::string_view name,
                                   const std::vector<FastToken>& arguments,
                                   size_t expected) {
    if (arguments.size() != expected) {
      throw std::runtime_error{std::string{"parameters expected: "} +
                               std::to_string(expected)};
    }
  }

  FastToken AppendNode(FastNode node) {
    nodes_.push_back(std::move(node));
    return FastToken(this, nodes_.size() - 1);
  }

  FastToken MakeUnaryFunction(FastFunctionKind function, FastToken argument) {
    FastNode node;
    node.kind = FastNodeKind::FunctionUnary;
    node.function = function;
    node.left = argument.node_index();
    return AppendNode(std::move(node));
  }

  FastToken MakeBinaryFunction(FastFunctionKind function,
                               FastToken left,
                               FastToken right) {
    FastNode node;
    node.kind = FastNodeKind::FunctionBinary;
    node.function = function;
    node.left = left.node_index();
    node.right = right.node_index();
    return AppendNode(std::move(node));
  }

  FastToken MakeFoldedFunction(FastFunctionKind function,
                               std::vector<FastToken> arguments) {
    if (arguments.size() == 1) {
      if (function == FastFunctionKind::Min || function == FastFunctionKind::Max ||
          function == FastFunctionKind::Or || function == FastFunctionKind::And) {
        return MakeUnaryFunction(function, std::move(arguments.front()));
      }
      return std::move(arguments.front());
    }

    if (function == FastFunctionKind::Or || function == FastFunctionKind::And) {
      FastToken folded = MakeBinaryFunction(function, arguments[arguments.size() - 2],
                                           arguments[arguments.size() - 1]);
      for (size_t i = arguments.size() - 2; i-- > 0;) {
        folded = MakeBinaryFunction(function, arguments[i], folded);
      }
      return folded;
    }

    FastToken folded = MakeBinaryFunction(function, arguments[0], arguments[1]);
    for (size_t i = 2; i < arguments.size(); ++i)
      folded = MakeBinaryFunction(function, folded, arguments[i]);
    return folded;
  }

  void AppendFunctionArguments(size_t node_index,
                               const FormatterDelegate& delegate,
                               std::string& str) const {
    const auto& node = nodes_[node_index];
    if (node.kind == FastNodeKind::FunctionBinary) {
      const auto& left = nodes_[node.left];
      if (left.kind == FastNodeKind::FunctionBinary && left.function == node.function) {
        AppendFunctionArguments(node.left, delegate, str);
      } else {
        FormatNode(node.left, delegate, str);
      }
      str += ", ";
      const auto& right = nodes_[node.right];
      if (right.kind == FastNodeKind::FunctionBinary &&
          right.function == node.function &&
          (node.function == FastFunctionKind::Or ||
           node.function == FastFunctionKind::And)) {
        AppendFunctionArguments(node.right, delegate, str);
      } else {
        FormatNode(node.right, delegate, str);
      }
      return;
    }

    if (node.kind == FastNodeKind::FunctionUnary) {
      FormatNode(node.left, delegate, str);
      return;
    }

    FormatNode(node_index, delegate, str);
  }

  Value CalculateNode(size_t node_index, void* data) const {
    const auto& node = nodes_[node_index];
    switch (node.kind) {
      case FastNodeKind::Literal:
        return node.literal;
      case FastNodeKind::UnaryOperator: {
        auto value = CalculateNode(node.left, data);
        switch (node.oper) {
          case '-':
            return -value;
          case '!':
            return !value;
          default:
            throw std::runtime_error{"unexpected unary operator"};
        }
      }
      case FastNodeKind::BinaryOperator: {
        auto value = CalculateNode(node.left, data);
        auto right = CalculateNode(node.right, data);
        switch (node.oper) {
          case '+':
            value += right;
            break;
          case '-':
            value -= right;
            break;
          case '*':
            value *= right;
            break;
          case '/':
            value /= right;
            break;
          case '^':
            value = std::pow(static_cast<double>(value), static_cast<double>(right));
            break;
          case '=':
            value = value == right;
            break;
          case '<':
            value = value < right;
            break;
          case '>':
            value = value > right;
            break;
          case 'l':
            value = value <= right;
            break;
          case 'g':
            value = value >= right;
            break;
          default:
            throw std::runtime_error{"unexpected binary operator"};
        }
        return value;
      }
      case FastNodeKind::Parentheses:
        return CalculateNode(node.left, data);
      case FastNodeKind::FunctionUnary: {
        auto value = CalculateNode(node.left, data);
        switch (node.function) {
          case FastFunctionKind::Abs:
            return std::abs(static_cast<double>(value));
          case FastFunctionKind::Not:
            return !static_cast<bool>(value);
          case FastFunctionKind::Sign: {
            const double x = static_cast<double>(value);
            if (std::abs(x) < Value::kPrecision)
              return 0.0;
            return x > 0.0 ? 1.0 : -1.0;
          }
          case FastFunctionKind::Sqrt:
            return std::sqrt(static_cast<double>(value));
          case FastFunctionKind::Sin:
            return std::sin(static_cast<double>(value));
          case FastFunctionKind::Cos:
            return std::cos(static_cast<double>(value));
          case FastFunctionKind::Tan:
            return std::tan(static_cast<double>(value));
          case FastFunctionKind::ASin:
            return std::asin(static_cast<double>(value));
          case FastFunctionKind::ACos:
            return std::acos(static_cast<double>(value));
          case FastFunctionKind::ATan:
            return std::atan(static_cast<double>(value));
          case FastFunctionKind::Min:
          case FastFunctionKind::Max:
          case FastFunctionKind::Or:
          case FastFunctionKind::And:
            return value;
          default:
            throw std::runtime_error{"unexpected unary function"};
        }
      }
      case FastNodeKind::FunctionBinary: {
        auto left = CalculateNode(node.left, data);
        switch (node.function) {
          case FastFunctionKind::Or:
            if (static_cast<bool>(left))
              return true;
            return static_cast<bool>(CalculateNode(node.right, data));
          case FastFunctionKind::And:
            if (!static_cast<bool>(left))
              return false;
            return static_cast<bool>(CalculateNode(node.right, data));
          default:
            break;
        }

        auto right = CalculateNode(node.right, data);
        switch (node.function) {
          case FastFunctionKind::ATan2:
            return std::atan2(static_cast<double>(left), static_cast<double>(right));
          case FastFunctionKind::BitXor: {
            const bool a = static_cast<bool>(left);
            const bool b = static_cast<bool>(right);
            return a ^ b;
          }
          case FastFunctionKind::Min:
            return left < right ? left : right;
          case FastFunctionKind::Max:
            return left < right ? right : left;
          default:
            throw std::runtime_error{"unexpected binary function"};
        }
      }
      case FastNodeKind::Conditional: {
        const auto condition = CalculateNode(node.left, data);
        return condition ? CalculateNode(node.right, data)
                         : CalculateNode(node.extra, data);
      }
    }

    throw std::runtime_error{"unexpected fast node"};
  }

  std::vector<FastNode> nodes_;
};

inline Value FastToken::Calculate(void* data) const {
  return storage_->Calculate(*this, data);
}

inline void FastToken::Traverse(TraverseCallback callback, void* param) const {
  storage_->Traverse(*this, callback, param);
}

inline void FastToken::Format(const FormatterDelegate& delegate,
                              std::string& str) const {
  str += storage_->Format(*this, delegate);
}

class FastParserDelegate {
 public:
  explicit FastParserDelegate(FastExpressionStorage& storage) : storage_{storage} {}

  FastToken MakeDoubleToken(double value) { return storage_.MakeDoubleToken(value); }
  FastToken MakeStringToken(std::string_view str) {
    return storage_.MakeStringToken(str);
  }

  template <class OperandToken>
  FastToken MakeUnaryOperatorToken(char oper, OperandToken&& operand_token) {
    return storage_.MakeUnaryOperatorToken(oper,
                                           std::forward<OperandToken>(operand_token));
  }

  template <class NestedToken>
  FastToken MakeParenthesesToken(NestedToken&& nested_token) {
    return storage_.MakeParenthesesToken(std::forward<NestedToken>(nested_token));
  }

  template <class LeftOperand, class RightOperand>
  FastToken MakeBinaryOperatorToken(char oper,
                                    LeftOperand&& left_operand,
                                    RightOperand&& right_operand) {
    return storage_.MakeBinaryOperatorToken(
        oper, std::forward<LeftOperand>(left_operand),
        std::forward<RightOperand>(right_operand));
  }

  FastToken MakeFunctionToken(std::string_view name,
                              std::vector<FastToken> arguments) {
    return storage_.MakeFunctionToken(name, std::move(arguments));
  }

  template <class Lexem, class Parser>
  FastToken MakeCustomToken(const Lexem&, Parser&) {
    throw std::runtime_error{"unexpected token"};
  }

 private:
  FastExpressionStorage& storage_;
};

}  // namespace detail

}  // namespace expression
