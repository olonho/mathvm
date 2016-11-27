#include "stack.h"

using namespace mathvm;

void TypedStack::swap() {
  VarType upperType = _types.top();
  _types.pop();
  VarType lowerType = _types.top();
  _types.pop();

  ValueUnion upper = _stack.top();
  _stack.pop();
  ValueUnion lower = _stack.top();
  _stack.pop();

  _types.push(upperType);
  _types.push(lowerType);

  _stack.push(upper);
  _stack.push(lower);
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
