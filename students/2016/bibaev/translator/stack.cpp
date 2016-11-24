#include "stack.h"

using namespace mathvm;

template<typename T>
void TypedStack::swapInternal(VarType lowerType) {
  switch (lowerType) {
    case VT_DOUBLE:
      swapTyped<T, double>();
      break;
    case VT_INT:
      swapTyped<T, int64_t>();
      break;
    case VT_STRING:
      swapTyped<T, int64_t>();
      break;
    default:
      throw std::runtime_error("wrong type on stack");
  }
}

void TypedStack::swap() {
  VarType upperType = _types.top();
  VarType lowerType = _types.top();

  switch (upperType) {
    case VT_DOUBLE:
      swapInternal<double>(lowerType);
      break;
    case VT_INT:
      swapInternal<int64_t>(lowerType);
      break;
    case VT_STRING:
      swapInternal<uint16_t>(lowerType);
      break;
    default:
      throw std::runtime_error("wrong element type on stack");
  }

  _types.push(upperType);
  _types.push(lowerType);
}

void TypedStack::popTop() {
  switch (_types.top()) {
    case VT_DOUBLE:
      popDouble();
      break;
    case VT_INT:
      popInt();
      break;
    case VT_STRING:
      popUInt16();
      break;
    default:
      throw std::runtime_error("unknown type on top of stack");
  }
}

template<typename UPPER_TYPE, typename LOWER_TYPE>
void TypedStack::swapTyped() {
  UPPER_TYPE upper = pop<UPPER_TYPE>();
  LOWER_TYPE lower = pop<LOWER_TYPE>();

  push<UPPER_TYPE>(upper);
  push<LOWER_TYPE>(lower);
}

template<typename T>
union U {
  T value;
  uint8_t byte[sizeof(T)];

  U(T val) : value(val) {}
};

template<typename T>
T TypedStack::pop() {
  U<T> result(0.);
  for (int i = sizeof(T) - 1; i >= 0; --i) {
    result.byte[i] = _stack.top();
    _stack.pop();
  }

  _types.pop();

  return result.value;
}

template<typename T>
void TypedStack::push(T value) {
  U<T> val = U<T>(value);
  for (size_t i = 0; i < sizeof(T); ++i) {
    _stack.push(val.byte[i]);
  }
}
