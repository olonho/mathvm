#include <iostream>
#include <ast.h>
#include <unordered_map>
#include <asmjit/base/globals.h>
#include "stack.h"
#include "my_interpreter.h"
#include "bytecode_stream.h"

using namespace mathvm;

namespace {
union ValueUnion {
  double floatingPointValue;
  int64_t integerValue;
  uint16_t stringId;

  ValueUnion(int64_t value) : integerValue(value) {};

  ValueUnion(double value) : floatingPointValue(value) {};

  ValueUnion(uint16_t value) : stringId(value) {};
};
}

template<typename T>
static int64_t comparePrimitive(T upper, T lower) {
  if (upper == lower) {
    return 0;
  }

  return upper > lower ? 1 : -1;
}

class VariableContainer {
private:
  struct VariableValue {
    ValueUnion value;
    VarType type;

    VariableValue() : value(0.0), type(VT_INVALID) {}

    VariableValue(ValueUnion val, VarType t) : value(val), type(t) {}
  };

  struct ContextDescription {
    const uint16_t id;
    const uint32_t localsNumber;
  };

public:
  void pushScope(uint16_t id, uint32_t localsNumber) {
    if (_contexts.size() > 0) {
      saveCache();
    }

    _contexts.push(ContextDescription{id, localsNumber});
    if (id >= _variables.size()) {
      _variables.resize(id + 1);
    }
    _variables[id].push(std::vector<VariableValue>(localsNumber));

    prepareCache();
  }

  void popScope() {
    _variables[_contexts.top().id].pop();
    _contexts.pop();
    prepareCache();
  }

  inline double getCachedDouble(uint16_t id) const {
    assert(id < 4 && _cacheTypes[id] == VT_DOUBLE);
    return _doubleRegister[id];
  }

  inline int64_t getCachedInt(uint16_t id) const {
    assert(id < 4 && _cacheTypes[id] == VT_INT);
    return _intRegister[id];
  }

  inline uint16_t getCachedString(uint16_t id) const {
    assert(id < 4 && _cacheTypes[id] == VT_STRING);
    return _stringRegister[id];
  }

  inline void setCachedDouble(uint16_t id, double value) {
    assert(id < 4);
    _doubleRegister[id] = value;
    _cacheTypes[id] = VT_DOUBLE;
  }

  inline void setCachedInt(uint16_t id, int64_t value) {
    assert(id < 4);
    _intRegister[id] = value;
    _cacheTypes[id] = VT_INT;
  }

  inline void setCachedString(uint16_t id, uint16_t value) {
    assert(id < 4);
    _stringRegister[id] = value;
    _cacheTypes[id] = VT_STRING;
  }

  double getDoubleVariableValue(uint16_t contextId, uint16_t variableId) {
    if (variableId < 4u && contextId == _contexts.top().id) {
      return getCachedDouble(variableId);
    }
    VariableValue& variable = getVariable(contextId, variableId);
    assert(variable.type == VT_DOUBLE);

    return variable.value.floatingPointValue;
  }

  int64_t getIntVariableValue(uint16_t contextId, uint16_t variableId) {
    if (variableId < 4u && contextId == _contexts.top().id) {
      return getCachedInt(variableId);
    }

    VariableValue& variable = getVariable(contextId, variableId);
    assert(variable.type == VT_INT);
    return variable.value.integerValue;
  }

  uint16_t getUInt16VariableValue(uint16_t contextId, uint16_t variableId) {
    if (variableId < 4u && contextId == _contexts.top().id) {
      return getCachedString(variableId);
    }

    VariableValue& variable = getVariable(contextId, variableId);
    assert(variable.type == VT_STRING);
    return variable.value.stringId;
  }

  void putDoubleVariableValue(uint16_t contextId, uint16_t variableId, double value) {
    getVariable(contextId, variableId) = VariableValue{ValueUnion{value}, VT_DOUBLE};
  }

  void putIntVariableValue(uint16_t contextId, uint16_t variableId, int64_t value) {
    getVariable(contextId, variableId) = VariableValue{ValueUnion{value}, VT_INT};
  }

  void putUInt16VariableValue(uint16_t contextId, uint16_t variableId, uint16_t value) {
    getVariable(contextId, variableId) = VariableValue{ValueUnion{value}, VT_STRING};
  }


private:
  inline void prepareCache() {
    ContextDescription const& contextDescription = _contexts.top();
    uint32_t cacheSize = std::min(4u, contextDescription.localsNumber);
    for (uint16_t i = 0; i < cacheSize; ++i) {
      VariableValue& variable = getVariable(contextDescription.id, i);
      _cacheTypes[i] = variable.type;
      switch (_cacheTypes[i]) {
        case VT_DOUBLE:
          _doubleRegister[i] = variable.value.floatingPointValue;
          break;
        case VT_INT:
          _intRegister[i] = variable.value.integerValue;
          break;
        case VT_STRING:
          _stringRegister[i] = variable.value.stringId;
          break;
        default:
          _cacheTypes[i] = VT_INVALID;
          break;
      }
    }
  }

  inline void saveCache() {
    ContextDescription const& contextDescription = _contexts.top();
    uint32_t cacheSize = std::min(4u, contextDescription.localsNumber);
    for (uint16_t i = 0; i < cacheSize; ++i) {
      switch (_cacheTypes[i]) {
        case VT_DOUBLE:
          putDoubleVariableValue(contextDescription.id, i, _doubleRegister[i]);
          break;
        case VT_INT:
          putIntVariableValue(contextDescription.id, i, _intRegister[i]);
          break;
        case VT_STRING:
          putUInt16VariableValue(contextDescription.id, i, _stringRegister[i]);
          break;
        default:
          getVariable(contextDescription.id, i).type = VT_INVALID;
      }
    }
  }

  VariableValue& getVariable(uint16_t contextId, uint16_t variableId) {
    return _variables[contextId].top()[variableId];
  }

  int64_t _intRegister[4];
  double _doubleRegister[4];
  uint16_t _stringRegister[4];
  VarType _cacheTypes[4];

  std::stack<ContextDescription> _contexts;

  std::vector<std::stack<std::vector<VariableValue>>> _variables;
};

struct BytecodeEvaluator {


  BytecodeEvaluator(std::ostream& out, InterpreterCodeImpl* code) :
      _code(code), _out(out) {
  }

  void pushBackToData(vector<uint8_t>& data, VarType type, ValueUnion value) {
    size_t size = data.size();
    switch (type) {
      case VT_INT: {
        data.resize(size + sizeof(int64_t));
        int64_t* ptr = (int64_t*) (data.data() + size);
        *ptr = value.integerValue;
        break;
      }
      case VT_DOUBLE: {
        data.resize(size + sizeof(int64_t));
        double* ptr = (double*) (data.data() + size);
        *ptr = value.floatingPointValue;
        break;
      }
      case VT_STRING: {
        data.resize(size + sizeof(int64_t));
        const char* c_str = _code->constantById(value.stringId).data();
        const char** ptr = (const char**) (data.data() + size);
        *ptr = c_str;
        break;
      }
      case VT_VOID: {
        break;
      }
      default:
        break;
    }
  }

  void callNative(uint16_t functionId) {
    const Signature* signature;
    const string* name;
    const void* nativePtr = _code->nativeById(functionId, &signature, &name);
    mathvm::VarType retType = signature->at(0).first;

    vector<uint8_t> mem;
    pushBackToData(mem, retType, ValueUnion(0.0));
    for (uint16_t j = 0; j < signature->size() - 1; ++j) {
      auto var = signature->at(j + 1);
      switch (var.first) {
        case VT_DOUBLE:
          pushBackToData(mem, VT_DOUBLE, _variables.getDoubleVariableValue(_currentContextId, j));
          break;
        case VT_INT:
          pushBackToData(mem, VT_INT, _variables.getIntVariableValue(_currentContextId, j));
          break;
        case VT_STRING:
          pushBackToData(mem, VT_STRING, _variables.getUInt16VariableValue(_currentContextId, j));
          break;
        default:
          break;
      }
    }

    typedef double (* double_handler)(void const*);
    typedef int64_t (* int_handler)(void const*);
    typedef const char* (* str_handler)(void const*);
    typedef void (* void_handler)(void const*);
    switch (retType) {
      case VT_DOUBLE: {
        double result = asmjit_cast<double_handler>(nativePtr)((void const*) mem.data());
        _stack.pushDouble(result);
        break;
      }
      case VT_INT: {
        int64_t result = asmjit_cast<int_handler>(nativePtr)((void const*) mem.data());
        _stack.pushInt(result);
      }
        break;
      case VT_STRING: {
        const char* c_str = asmjit_cast<str_handler>(nativePtr)((void const*) mem.data());
        uint16_t id = _code->makeStringConstant(string(c_str));
        _stack.pushUInt16(id);
      }
        break;
      case VT_VOID:
        asmjit_cast<void_handler>(nativePtr)((void const*) mem.data());
        break;
      default:
        throw runtime_error(std::string("unexpected return type") + typeToName(retType));
    }
  }

  void evaluate(BytecodeFunction* function) {
    _currentContextId = function->id();

    _variables.pushScope(function->id(), function->localsNumber());

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
          _stack.pushDouble(_variables.getCachedDouble(0));
          break;
        case BC_LOADDVAR1:
          _stack.pushDouble(_variables.getCachedDouble(1));
          break;
        case BC_LOADDVAR2:
          _stack.pushDouble(_variables.getCachedDouble(2));
          break;
        case BC_LOADDVAR3:
          _stack.pushDouble(_variables.getCachedDouble(3));
          break;
        case BC_LOADIVAR0:
          _stack.pushInt(_variables.getCachedInt(0));
          break;
        case BC_LOADIVAR1:
          _stack.pushInt(_variables.getCachedInt(1));
          break;
        case BC_LOADIVAR2:
          _stack.pushInt(_variables.getCachedInt(2));
          break;
        case BC_LOADIVAR3:
          _stack.pushInt(_variables.getCachedInt(3));
          break;
        case BC_LOADSVAR0:
          _stack.pushUInt16(_variables.getCachedString(0));
          break;
        case BC_LOADSVAR1:
          _stack.pushUInt16(_variables.getCachedString(1));
          break;
        case BC_LOADSVAR2:
          _stack.pushUInt16(_variables.getCachedString(2));
          break;
        case BC_LOADSVAR3:
          _stack.pushUInt16(_variables.getCachedString(3));
          break;
        case BC_STOREDVAR0:
          _variables.setCachedDouble(0, _stack.popDouble());
          break;
        case BC_STOREDVAR1:
          _variables.setCachedDouble(1, _stack.popDouble());
          break;
        case BC_STOREDVAR2:
          _variables.setCachedDouble(2, _stack.popDouble());
          break;
        case BC_STOREDVAR3:
          _variables.setCachedDouble(3, _stack.popDouble());
          break;
        case BC_STOREIVAR0:
          _variables.setCachedInt(0, _stack.popInt());
          break;
        case BC_STOREIVAR1:
          _variables.setCachedInt(1, _stack.popInt());
          break;
        case BC_STOREIVAR2:
          _variables.setCachedInt(2, _stack.popInt());
          break;
        case BC_STOREIVAR3:
          _variables.setCachedInt(3, _stack.popInt());
          break;
        case BC_STORESVAR0:
          _variables.setCachedString(0, _stack.popUInt16());
          break;
        case BC_STORESVAR1:
          _variables.setCachedString(1, _stack.popUInt16());
          break;
        case BC_STORESVAR2:
          _variables.setCachedString(2, _stack.popUInt16());
          break;
        case BC_STORESVAR3:
          _variables.setCachedString(3, _stack.popUInt16());
          break;
        case BC_LOADDVAR: {
          uint16_t variableId = bytecodeStream.readUInt16();
          _stack.pushDouble(_variables.getDoubleVariableValue(_currentContextId, variableId));
          break;
        }
        case BC_LOADIVAR: {
          uint16_t contextId = _currentContextId;
          uint16_t variableId = bytecodeStream.readUInt16();
          _stack.pushInt(_variables.getIntVariableValue(contextId, variableId));
          break;
        }
        case BC_LOADSVAR: {
          uint16_t contextId = _currentContextId;
          uint16_t variableId = bytecodeStream.readUInt16();
          _stack.pushUInt16(_variables.getUInt16VariableValue(contextId, variableId));
          break;
        }
        case BC_STOREDVAR: {
          uint16_t contextId = _currentContextId;
          uint16_t variableId = bytecodeStream.readUInt16();
          double value = _stack.popDouble();
          _variables.putDoubleVariableValue(contextId, variableId, value);
          break;
        }
        case BC_STOREIVAR: {
          uint16_t contextId = _currentContextId;
          uint16_t variableId = bytecodeStream.readUInt16();
          int64_t value = _stack.popInt();
          _variables.putIntVariableValue(contextId, variableId, value);
          break;
        }
        case BC_STORESVAR: {
          uint16_t contextId = _currentContextId;
          uint16_t variableId = bytecodeStream.readUInt16();
          uint16_t value = _stack.popUInt16();
          _variables.putUInt16VariableValue(contextId, variableId, value);
          break;
        }
        case BC_LOADCTXDVAR: {
          uint16_t contextId = bytecodeStream.readUInt16();
          uint16_t variableId = bytecodeStream.readUInt16();
          _stack.pushDouble(_variables.getDoubleVariableValue(contextId, variableId));
          break;
        }
        case BC_LOADCTXIVAR: {
          uint16_t contextId = bytecodeStream.readUInt16();
          uint16_t variableId = bytecodeStream.readUInt16();
          _stack.pushInt(_variables.getIntVariableValue(contextId, variableId));
          break;
        }
        case BC_LOADCTXSVAR: {
          uint16_t contextId = bytecodeStream.readUInt16();
          uint16_t variableId = bytecodeStream.readUInt16();
          _stack.pushUInt16(_variables.getUInt16VariableValue(contextId, variableId));
          break;
        }
        case BC_STORECTXDVAR: {
          uint16_t contextId = bytecodeStream.readUInt16();
          uint16_t variableId = bytecodeStream.readUInt16();
          double value = _stack.popDouble();
          _variables.putDoubleVariableValue(contextId, variableId, value);
          break;
        }
        case BC_STORECTXIVAR: {
          uint16_t contextId = bytecodeStream.readUInt16();
          uint16_t variableId = bytecodeStream.readUInt16();
          int64_t value = _stack.popInt();
          _variables.putIntVariableValue(contextId, variableId, value);
          break;
        }
        case BC_STORECTXSVAR: {
          uint16_t contextId = bytecodeStream.readUInt16();
          uint16_t variableId = bytecodeStream.readUInt16();
          uint16_t value = _stack.popUInt16();
          _variables.putUInt16VariableValue(contextId, variableId, value);
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
          uint16_t functionId = bytecodeStream.readUInt16();
          BytecodeFunction* calledFunction = _code->functionById(functionId);
          uint16_t oldContextId = _currentContextId;
          evaluate(calledFunction);
          _variables.popScope();
          _currentContextId = oldContextId;

          break;
        }
        case BC_CALLNATIVE: {
          uint16_t functionId = bytecodeStream.readUInt16();
          callNative(functionId);
          break;
        }
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

  VariableContainer _variables;
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
