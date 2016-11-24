#pragma once

#include <cstdint>
#include <stack>
#include <cstddef>
#include <mathvm.h>

class TypedStack {
public:
  inline double popDouble() {
    assert(_types.top() == mathvm::VT_DOUBLE);
    return pop<double>();
  }

  inline int64_t popInt() {
    assert(_types.top() == mathvm::VT_INT);
    return pop<int64_t>();
  }

  inline uint16_t popUInt16() {
    assert(_types.top() == mathvm::VT_STRING);
    return pop<uint16_t>();
  }

  inline void pushDouble(double val) {
    push(val);
    _types.push(mathvm::VT_DOUBLE);
  }

  inline void pushInt(int64_t val) {
    push(val);
    _types.push(mathvm::VT_INT);
  }

  inline void pushUInt16(uint16_t val) {
    push(val);
    _types.push(mathvm::VT_STRING);
  }

  inline mathvm::VarType topOfStackType() const {
    return _types.top();
  }

  void swap();

  void popTop();

private:

  template<typename T>
  void swapInternal(mathvm::VarType lowerType);

  template<typename UPPER_TYPE, typename LOWER_TYPE>
  void swapTyped();

  template<typename T>
  T pop();

  template<typename T>
  void push(T value);

  std::stack<uint8_t> _stack;
  std::stack<mathvm::VarType> _types;
};
