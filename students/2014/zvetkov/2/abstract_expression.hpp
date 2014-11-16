#ifndef ABSTRACT_EXPRESSION_HPP
#define ABSTRACT_EXPRESSION_HPP 

#include "ast.h"
#include "mathvm.h"
#include "error_reporter.hpp"

#include <stack>
#include <vector>
#include <utility>

namespace mathvm {

class ExpressionType {
  typedef std::stack<VarType> ExptessionTypeStack;

  ExptessionTypeStack expressionType_;
  ErrorReporter& reporter_;
  bool alternatives_;
  bool matched_;
public:
  ExpressionType(ErrorReporter& reporter): reporter_(reporter), alternatives_(false), matched_(false) {}
  
  ~ExpressionType() {
    //TODO assert(expressionType_.empty());
  }

  void load(VarType type) {
    assert(!alternatives_);
    expressionType_.push(type);
  }

  void consume(VarType type, AstNode* node) {
    if (tosType() != type) {
      reporter_.error("Unsupported operand type ", node);
    }

    expressionType_.pop();
  }

  VarType pop() {
    VarType top = tosType();
    expressionType_.pop();
    return top;
  }

  void swap() {
    VarType upperType = tosType();
    expressionType_.pop();
    VarType lowerType = tosType();
    expressionType_.pop();
    expressionType_.push(upperType);
    expressionType_.push(lowerType);
  }

  void alternatives() {
    assert(!alternatives_);
    alternatives_ = true;
    matched_ = false;
  }

  void assertEmpty(AstNode* node) {
    if (!expressionType_.empty()) {
      reporter_.error("Internal error: not empty expression stack ", node);
    }
  }

  bool unOp(VarType operandType, VarType resultType) {
    assert(alternatives_);
    if (matched_ || tosType() != operandType) { return false; }

    matched_ = true;

    if (operandType != resultType) {
      expressionType_.pop();
      expressionType_.push(resultType);
    }

    return true;
  }

  bool binOp(VarType upperType, VarType lowerType, VarType resultType) {
    assert(alternatives_);
    if (matched_ || tosType() != upperType) { return false; }

    expressionType_.pop();
    if (tosType() != lowerType) {
      expressionType_.push(upperType);
      return false;
    }

    matched_ = true;
    expressionType_.pop();
    expressionType_.push(resultType);
    return true;
  }

  bool apply(AstNode* node) {
    assert(alternatives_);
    alternatives_ = false;

    if (!matched_) {
      reporter_.error("Unsupported operation types", node);
    }

    return matched_;
  }

private:
  VarType tosType() {
    assert(!expressionType_.empty());
    return expressionType_.top();
  }
};

}

#endif