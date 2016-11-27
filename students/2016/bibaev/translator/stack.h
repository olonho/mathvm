#pragma once

#include <cstdint>
#include <stack>
#include <cstddef>
#include <mathvm.h>

class TypedStack {
public:
  inline double popDouble() {
    assert(_types.top() == mathvm::VT_DOUBLE);
    _types.pop();
    return popVal().floating;
  }

  inline int64_t popInt() {
    assert(_types.top() == mathvm::VT_INT);
    _types.pop();
    return popVal().integer;
  }

  inline uint16_t popUInt16() {
    assert(_types.top() == mathvm::VT_STRING);
    _types.pop();
    return popVal().string;
  }

  inline void pushDouble(double val) {
    _stack.push(ValueUnion(val));
    _types.push(mathvm::VT_DOUBLE);
  }

  inline void pushInt(int64_t val) {
    _stack.push(ValueUnion(val));
    _types.push(mathvm::VT_INT);
  }

  inline void pushUInt16(uint16_t val) {
    _stack.push(ValueUnion(val));
    _types.push(mathvm::VT_STRING);
  }

  inline mathvm::VarType topOfStackType() const {
    return _types.top();
  }

  void swap();

  void popTop();

private:

  union ValueUnion {
    int64_t integer;
    double floating;
    uint16_t string;

    ValueUnion(int64_t value) : integer(value) {};

    ValueUnion(double value) : floating(value) {};

    ValueUnion(uint16_t value) : string(value) {};
  };

  inline ValueUnion popVal() {
    ValueUnion val = _stack.top();
    _stack.pop();
    return val;
  }

  std::stack<ValueUnion> _stack;
  std::stack<mathvm::VarType> _types;
};
