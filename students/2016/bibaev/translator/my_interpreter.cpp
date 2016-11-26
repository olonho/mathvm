#include <iostream>
#include <ast.h>
#include <unordered_map>
#include "stack.h"
#include "my_interpreter.h"
#include "bytecode_stream.h"

using namespace mathvm;

template<typename T>
static int64_t comparePrimitive(T upper, T lower) {
  if (upper == lower) {
    return 0;
  }

  return upper > lower ? 1 : -1;
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
      Instruction instruction = bytecodeStream.readInstruction();
      switch (instruction) {
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

#define BIN_OP_DOUBLE(op) {\
          double upper = _stack.popDouble();\
          double lower = _stack.popDouble();\
          _stack.pushDouble(upper op lower);\
          break;\
      }

        case BC_DADD: BIN_OP_DOUBLE(+)
        case BC_DSUB: BIN_OP_DOUBLE(-)
        case BC_DMUL: BIN_OP_DOUBLE(*)
        case BC_DDIV: BIN_OP_DOUBLE(/)

#undef BIN_OP_DOUBLE

#define BIN_OP_INT(op) {\
          int64_t upper = _stack.popInt();\
          int64_t lower = _stack.popInt();\
          _stack.pushInt(upper op lower);\
          break;\
      }

        case BC_IADD: BIN_OP_INT(+)
        case BC_ISUB: BIN_OP_INT(-)
        case BC_IMUL: BIN_OP_INT(*)
        case BC_IDIV: BIN_OP_INT(/)
        case BC_IMOD: BIN_OP_INT(%)
        case BC_IAOR: BIN_OP_INT(|)
        case BC_IAAND: BIN_OP_INT(&)
        case BC_IAXOR: BIN_OP_INT(^)

#undef BIN_OP_INT

        case BC_DNEG:
          _stack.pushDouble(-_stack.popDouble());
          break;
        case BC_INEG:
          _stack.pushInt(-_stack.popInt());
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
        case BC_LOADDVAR: {
          uint16_t contextId = _currentContextId;
          uint16_t variableId = bytecodeStream.readUInt16();
          _stack.pushDouble(getDoubleVariable(contextId, variableId));
          break;
        }
        case BC_LOADIVAR: {
          uint16_t contextId = _currentContextId;
          uint16_t variableId = bytecodeStream.readUInt16();
          _stack.pushInt(getIntVariable(contextId, variableId));
          break;
        }
        case BC_LOADSVAR: {
          uint16_t contextId = _currentContextId;
          uint16_t variableId = bytecodeStream.readUInt16();
          _stack.pushUInt16(getUIntVariable(contextId, variableId));
          break;
        }
        case BC_STOREDVAR: {
          uint16_t contextId = _currentContextId;
          uint16_t variableId = bytecodeStream.readUInt16();
          double value = _stack.popDouble();
          putDoubleVariable(contextId, variableId, value);
          break;
        }
        case BC_STOREIVAR: {
          uint16_t contextId = _currentContextId;
          uint16_t variableId = bytecodeStream.readUInt16();
          int64_t value = _stack.popInt();
          putIntVariable(contextId, variableId, value);
          break;
        }
        case BC_STORESVAR: {
          uint16_t contextId = _currentContextId;
          uint16_t variableId = bytecodeStream.readUInt16();
          uint16_t value = _stack.popUInt16();
          putStringVariable(contextId, variableId, value);
          break;
        }
        case BC_LOADCTXDVAR: {
          uint16_t contextId = bytecodeStream.readUInt16();
          uint16_t variableId = bytecodeStream.readUInt16();
          _stack.pushDouble(getDoubleVariable(contextId, variableId));
          break;
        }
        case BC_LOADCTXIVAR: {
          uint16_t contextId = bytecodeStream.readUInt16();
          uint16_t variableId = bytecodeStream.readUInt16();
          _stack.pushInt(getIntVariable(contextId, variableId));
          break;
        }
        case BC_LOADCTXSVAR: {
          uint16_t contextId = bytecodeStream.readUInt16();
          uint16_t variableId = bytecodeStream.readUInt16();
          _stack.pushUInt16(getUIntVariable(contextId, variableId));
          break;
        }
        case BC_STORECTXDVAR: {
          uint16_t contextId = bytecodeStream.readUInt16();
          uint16_t variableId = bytecodeStream.readUInt16();
          double value = _stack.popDouble();
          putDoubleVariable(contextId, variableId, value);
          break;
        }
        case BC_STORECTXIVAR: {
          uint16_t contextId = bytecodeStream.readUInt16();
          uint16_t variableId = bytecodeStream.readUInt16();
          int64_t value = _stack.popInt();
          putIntVariable(contextId, variableId, value);
          break;
        }
        case BC_STORECTXSVAR: {
          uint16_t contextId = bytecodeStream.readUInt16();
          uint16_t variableId = bytecodeStream.readUInt16();
          uint16_t value = _stack.popUInt16();
          putStringVariable(contextId, variableId, value);
          break;
        }
        case BC_DCMP: {
          double upper = _stack.popDouble();
          double lower = _stack.popDouble();
          _stack.pushInt(comparePrimitive(upper, lower));
          break;
        }
        case BC_ICMP: {
          int64_t upper = _stack.popInt();
          int64_t lower = _stack.popInt();
          _stack.pushInt(comparePrimitive(upper, lower));
          break;
        }
        case BC_JA: {
          int16_t signedOffset = bytecodeStream.readInt16() - sizeof(int16_t);
          bytecodeStream.jump(signedOffset);
          break;
        }

#define CMP_INT(op) {\
          int64_t upper = _stack.popInt(); \
          int64_t lower = _stack.popInt(); \
          uint16_t signedOffset = bytecodeStream.readUInt16(); \
          if (comparePrimitive(upper, lower) op 0) { \
            bytecodeStream.jump(signedOffset - sizeof(uint16_t)); \
          } \
          _stack.pushInt(lower); \
          _stack.pushInt(upper);\
          break; \
      }

        case BC_IFICMPNE: CMP_INT(!=)
        case BC_IFICMPE: CMP_INT(==)
        case BC_IFICMPG: CMP_INT(>)
        case BC_IFICMPGE: CMP_INT(>=)
        case BC_IFICMPL: CMP_INT(<)
        case BC_IFICMPLE: CMP_INT(<=)
#undef CMP_INT
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

  double getDoubleVariable(uint16_t contextId, uint16_t variableId) {
    if (contextId == _currentContextId && variableId < 4) {
      return _doubleRegister[variableId];
    }

    assert(_context2VariableId2Type[contextId][variableId] == VT_DOUBLE);
    return _context2VariableId2Value[contextId][variableId].floatingPointValue;
  }

  int64_t getIntVariable(uint16_t contextId, uint16_t variableId) {
    if (contextId == _currentContextId && variableId < 4) {
      return _intRegister[variableId];
    }

    assert(_context2VariableId2Type[contextId][variableId] == VT_INT);
    return _context2VariableId2Value[contextId][variableId].integerValue;
  }

  uint16_t getUIntVariable(uint16_t contextId, uint16_t variableId) {
    if (contextId == _currentContextId && variableId < 4) {
      return _stringRegister[variableId];
    }

    assert(_context2VariableId2Type[contextId][variableId] == VT_STRING);
    return _context2VariableId2Value[contextId][variableId].stringId;
  }

  void putDoubleVariable(uint16_t contextId, uint16_t variableId, double val) {
    ValueUnion value;
    value.floatingPointValue = val;
    _context2VariableId2Value[contextId][variableId] = value;
    _context2VariableId2Type[contextId][variableId] = VT_DOUBLE;
  }

  void putIntVariable(uint16_t contextId, uint16_t variableId, int64_t val) {
    ValueUnion value;
    value.integerValue = val;
    _context2VariableId2Value[contextId][variableId] = value;
    _context2VariableId2Type[contextId][variableId] = VT_INT;
  }

  void putStringVariable(uint16_t contextId, uint16_t variableId, uint16_t val) {
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
  BytecodeFunction* topLevelFunction = functionByName(AstFunction::top_name);
  BytecodeEvaluator evaluator(_out, this);

  try {
    evaluator.evaluate(topLevelFunction);
  } catch (std::exception const& e) {
    return Status::Error(e.what());
  }

  return Status::Ok();
}
