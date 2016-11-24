#include <iostream>
#include <ast.h>
#include <unordered_map>
#include "stack.h"
#include "my_interpreter.h"
#include "bytecode_stream.h"

using namespace mathvm;

template<typename T>
static int64_t comparePrimitive(T left, T right) {
  if (left == right) {
    return 0;
  }

  return left < right ? -1 : 1;
}

struct BytecodeEvaluator {
  typedef union {
    double floatingPointValue;
    int64_t integerValue;
    uint16_t stringId;
  } ValueUnion;

  BytecodeEvaluator(std::ostream& out, InterpreterCodeImpl* code) :
      _out(out), _code(code) {
  }

  void evaluate(BytecodeFunction* function) {
    uint16_t scope = function->scopeId();
    function->localsNumber();
    _currentContextId = function->scopeId();

    BytecodeStream bytecodeStream(function->bytecode());
    while (bytecodeStream.hasNext()) {
      switch (bytecodeStream.readInstruction()) {
        case BC_INVALID:
          throw std::runtime_error("invalid instruction found");
        case BC_DLOAD:
          _stack.pushDouble(bytecodeStream.readDouble());
          break;
        case BC_ILOAD:
          _stack.pushInt(bytecodeStream.readInt64());
          break;
        case BC_SLOAD:
          _stack.pushUInt16(bytecodeStream.readUInt16());
          break;
        case BC_DLOAD0:
          _stack.pushDouble(0.);
          break;
        case BC_ILOAD0:
          _stack.pushInt(0);
          break;
        case BC_SLOAD0:
          _stack.pushUInt16(0);
          break;
        case BC_DLOAD1:
          _stack.pushDouble(1.);
          break;
        case BC_ILOAD1:
          _stack.pushInt(1);
          break;
        case BC_DLOADM1:
          _stack.pushDouble(-1.);
          break;
        case BC_ILOADM1:
          _stack.pushInt(-1);
          break;
        case BC_DADD:
          _stack.pushDouble(_stack.popDouble() + _stack.popDouble());
          break;
        case BC_IADD:
          _stack.pushInt(_stack.popInt() + _stack.popInt());
          break;
        case BC_DSUB: {
          double left = _stack.popDouble();
          double right = _stack.popDouble();
          _stack.pushDouble(left - right);
        }
          break;
        case BC_ISUB: {
          int64_t left = _stack.popInt();
          int64_t right = _stack.popInt();
          _stack.pushInt(left - right);
        }
          break;
        case BC_DMUL:
          _stack.pushDouble(_stack.popDouble() * _stack.popDouble());
          break;
        case BC_IMUL:
          _stack.pushInt(_stack.popInt() * _stack.popInt());
          break;
        case BC_DDIV: {
          double left = _stack.popDouble();
          double right = _stack.popDouble();
          _stack.pushDouble(left / right);
        }
          break;
        case BC_IDIV: {
          int64_t upper = _stack.popInt();
          int64_t lower = _stack.popInt();
          _stack.pushInt(upper / lower);
        }
          break;
        case BC_IMOD: {
          int64_t left = _stack.popInt();
          int64_t right = _stack.popInt();
          _stack.pushInt(left % right);
        }
          break;
        case BC_DNEG:
          _stack.pushDouble(-_stack.popDouble());
          break;
        case BC_INEG:
          _stack.pushInt(-_stack.popInt());
          break;
        case BC_IAOR:
          _stack.pushInt(_stack.popInt() | _stack.popInt());
          break;
        case BC_IAAND:
          _stack.pushInt(_stack.popInt() & _stack.popInt());
          break;
        case BC_IAXOR:
          _stack.pushInt(_stack.popInt() ^ _stack.popInt());
          break;
        case BC_IPRINT:
          _out << _stack.popInt();
          break;
        case BC_DPRINT:
          _out << _stack.popDouble();
          break;
        case BC_SPRINT:
          _out << _code->constantById(_stack.popUInt16());
          break;
        case BC_I2D:
          _stack.pushDouble(static_cast<double>(_stack.popInt()));
          break;
        case BC_D2I:
          _stack.pushInt(static_cast<int64_t>(_stack.popDouble()));
          break;
        case BC_S2I:
          _stack.pushInt(reinterpret_cast<int64_t>(&_code->constantById(_stack.popUInt16())));
          break;
        case BC_SWAP:
          _stack.swap();
          break;
        case BC_POP:
          _stack.popTop();
          break;
        case BC_LOADDVAR0:
          _stack.pushDouble(_doubleRegister[0]);
          break;
        case BC_LOADDVAR1:
          _stack.pushDouble(_doubleRegister[1]);
          break;
        case BC_LOADDVAR2:
          _stack.pushDouble(_doubleRegister[2]);
          break;
        case BC_LOADDVAR3:
          _stack.pushDouble(_doubleRegister[3]);
          break;
        case BC_LOADIVAR0:
          _stack.pushInt(_intRegister[0]);
          break;
        case BC_LOADIVAR1:
          _stack.pushInt(_intRegister[1]);
          break;
        case BC_LOADIVAR2:
          _stack.pushInt(_intRegister[2]);
          break;
        case BC_LOADIVAR3:
          _stack.pushInt(_intRegister[3]);
          break;
        case BC_LOADSVAR0:
          _stack.pushUInt16(_stringRegister[0]);
          break;
        case BC_LOADSVAR1:
          _stack.pushUInt16(_stringRegister[1]);
          break;
        case BC_LOADSVAR2:
          _stack.pushUInt16(_stringRegister[2]);
          break;
        case BC_LOADSVAR3:
          _stack.pushUInt16(_stringRegister[3]);
          break;
        case BC_STOREDVAR0:
          _doubleRegister[0] = _stack.popDouble();
          break;
        case BC_STOREDVAR1:
          _doubleRegister[1] = _stack.popDouble();
          break;
        case BC_STOREDVAR2:
          _doubleRegister[2] = _stack.popDouble();
          break;
        case BC_STOREDVAR3:
          _doubleRegister[3] = _stack.popDouble();
          break;
        case BC_STOREIVAR0:
          _intRegister[0] = _stack.popInt();
          break;
        case BC_STOREIVAR1:
          _intRegister[1] = _stack.popInt();
          break;
        case BC_STOREIVAR2:
          _intRegister[2] = _stack.popInt();
          break;
        case BC_STOREIVAR3:
          _intRegister[3] = _stack.popInt();
          break;
        case BC_STORESVAR0:
          _stringRegister[0] = _stack.popUInt16();
          break;
        case BC_STORESVAR1:
          _stringRegister[1] = _stack.popUInt16();
          break;
        case BC_STORESVAR2:
          _stringRegister[2] = _stack.popUInt16();
          break;
        case BC_STORESVAR3:
          _stringRegister[3] = _stack.popUInt16();
          break;
        case BC_LOADDVAR:
          // TODO: Do not run read action in function arguments
          _stack.pushDouble(loadDoubleVariable(_currentContextId, bytecodeStream.readUInt16()));
          break;
        case BC_LOADIVAR:
          _stack.pushInt(loadIntVariable(_currentContextId, bytecodeStream.readUInt16()));
          break;
        case BC_LOADSVAR:
          _stack.pushUInt16(loadUIntVariable(_currentContextId, bytecodeStream.readUInt16()));
          break;
        case BC_STOREDVAR:
          storeDoubleVariable(_currentContextId, bytecodeStream.readUInt16(), _stack.popDouble());
          break;
        case BC_STOREIVAR:
          storeIntVariable(_currentContextId, bytecodeStream.readUInt16(), _stack.popInt());
          break;
        case BC_STORESVAR:
          storeStringVariable(_currentContextId, bytecodeStream.readUInt16(), _stack.popUInt16());
          break;
        case BC_LOADCTXDVAR:
          _stack.pushDouble(loadDoubleVariable(bytecodeStream.readUInt16(), bytecodeStream.readUInt16()));
          break;
        case BC_LOADCTXIVAR:
          _stack.pushInt(loadIntVariable(bytecodeStream.readUInt16(), bytecodeStream.readUInt16()));
          break;
        case BC_LOADCTXSVAR:
          _stack.pushUInt16(loadUIntVariable(bytecodeStream.readUInt16(), bytecodeStream.readUInt16()));
          break;
        case BC_STORECTXDVAR:
          storeDoubleVariable(bytecodeStream.readUInt16(), bytecodeStream.readUInt16(), _stack.popDouble());
          break;
        case BC_STORECTXIVAR:
          storeIntVariable(bytecodeStream.readUInt16(), bytecodeStream.readUInt16(), _stack.popInt());
          break;
        case BC_STORECTXSVAR:
          storeStringVariable(bytecodeStream.readUInt16(), bytecodeStream.readUInt16(), _stack.popUInt16());
          break;
        case BC_DCMP:
          _stack.pushInt(comparePrimitive(_stack.popDouble(), _stack.popDouble()));
          break;
        case BC_ICMP:
          _stack.pushInt(comparePrimitive(_stack.popInt(), _stack.popInt()));
          break;
        case BC_JA: {
          bytecodeStream.skip(bytecodeStream.readUInt16());
          break;
        }
        case BC_IFICMPNE:
          if (comparePrimitive(_stack.popInt(), _stack.popInt()) != 0) {
            bytecodeStream.skip(bytecodeStream.readUInt16());
          }
          break;
        case BC_IFICMPE:
          if (comparePrimitive(_stack.popInt(), _stack.popInt()) == 0) {
            bytecodeStream.skip(bytecodeStream.readUInt16());
          }
          break;
        case BC_IFICMPG:
          if (comparePrimitive(_stack.popInt(), _stack.popInt()) > 0) {
            bytecodeStream.skip(bytecodeStream.readUInt16());
          }
          break;
        case BC_IFICMPGE:
          if (comparePrimitive(_stack.popInt(), _stack.popInt()) >= 0) {
            bytecodeStream.skip(bytecodeStream.readUInt16());
          }
          break;
        case BC_IFICMPL:
          if (comparePrimitive(_stack.popInt(), _stack.popInt()) < 0) {
            bytecodeStream.skip(bytecodeStream.readUInt16());
          }
          break;
        case BC_IFICMPLE:
          if (comparePrimitive(_stack.popInt(), _stack.popInt()) <= 0) {
            bytecodeStream.skip(bytecodeStream.readUInt16());
          }
          break;
        case BC_DUMP: {
          switch (_stack.topOfStackType()) {
            case mathvm::VT_DOUBLE: {
              double value = _stack.popDouble();
              _out << value;
              _stack.pushDouble(value);
              break;
            }
            case mathvm::VT_INT: {
              int64_t value = _stack.popInt();
              _out << value;
              _stack.pushInt(value);
              break;
            }
            case mathvm::VT_STRING: {
              uint16_t id = _stack.popUInt16();
              _out << _code->constantById(id);
              _stack.pushUInt16(id);
              break;
            }
            default:
              throw std::runtime_error("wrong type on the top of the stack");
          }
        }
        case BC_STOP:
          break;
        case BC_CALL: {
//          uint16_t functionId = _stack.popUInt16();
//          BytecodeFunction* function = _code->functionById(functionId);
          break;
        }
        case BC_CALLNATIVE:
          break;
        case BC_RETURN:
          return;
        case BC_BREAK:
          std::cerr << "no debugger support" << std::endl;
          break;
        case BC_LAST:
          throw std::runtime_error("wrong bytecode: BC_LAST");
      }
    }
  }

private:

  double loadDoubleVariable(uint16_t contextId, uint16_t variableId) {
    if (contextId == _currentContextId && variableId < 4) {
      return _doubleRegister[variableId];
    }

    assert(_context2VariableId2Type[contextId][variableId] == VT_DOUBLE);
    return _context2VariableId2Value[contextId][variableId].floatingPointValue;
  }

  int64_t loadIntVariable(uint16_t contextId, uint16_t variableId) {
    if (contextId == _currentContextId && variableId < 4) {
      return _intRegister[variableId];
    }

    assert(_context2VariableId2Type[contextId][variableId] == VT_INT);
    return _context2VariableId2Value[contextId][variableId].integerValue;
  }

  uint16_t loadUIntVariable(uint16_t contextId, uint16_t variableId) {
    if (contextId == _currentContextId && variableId < 4) {
      return _stringRegister[variableId];
    }

    assert(_context2VariableId2Type[contextId][variableId] == VT_STRING);
    return _context2VariableId2Value[contextId][variableId].stringId;
  }

  void storeDoubleVariable(uint16_t contextId, uint16_t variableId, double val) {
    ValueUnion value;
    value.floatingPointValue = val;
    _context2VariableId2Value[contextId][variableId] = value;
    _context2VariableId2Type[contextId][variableId] = VT_DOUBLE;
  }

  void storeIntVariable(uint16_t contextId, uint16_t variableId, int64_t val) {
    ValueUnion value;
    value.integerValue = val;
    _context2VariableId2Value[contextId][variableId] = value;
    _context2VariableId2Type[contextId][variableId] = VT_INT;
  }

  void storeStringVariable(uint16_t contextId, uint16_t variableId, uint16_t val) {
    ValueUnion value;
    value.stringId = val;
    _context2VariableId2Value[contextId][variableId] = value;
    _context2VariableId2Type[contextId][variableId] = VT_STRING;
  }

  std::unordered_map<uint16_t, std::unordered_map<uint16_t, ValueUnion>> _context2VariableId2Value;
  std::unordered_map<uint16_t, std::unordered_map<uint16_t, VarType>> _context2VariableId2Type;
  int64_t _intRegister[4];
  double _doubleRegister[4];
  uint16_t _stringRegister[4];

  uint16_t _currentContextId;
  TypedStack _stack;
  InterpreterCodeImpl* _code;
  std::ostream& _out;
};

Status* InterpreterCodeImpl::execute(std::vector<Var*>& vars) {
  std::cerr << "TODO: Implement interpreter!." << std::endl;
  BytecodeFunction* topLevelFunction = functionByName(AstFunction::top_name);
  BytecodeEvaluator evaluator(_out, this);

  evaluator.evaluate(topLevelFunction);
  return Status::Error("Not implemented. It is next homework");
}
