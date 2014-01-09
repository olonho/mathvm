#include <stdexcept>
#include <stdlib.h>

#include "InterpreterCodeImpl.h"
#include "DebugMacros.h"

namespace mathvm {

class Value {
public:
Value() : _type(VT_INVALID), _type_ptr(0) {
}

Value(int64_t intValue) : _type(VT_INT), _type_ptr(&_type) {
  setInt(intValue);
}

Value(double doubleValue) : _type(VT_DOUBLE), _type_ptr(&_type) {
  setDouble(doubleValue);
}

Value(char const * stringValue) : _type(VT_STRING), _type_ptr(&_type) {
  setString(stringValue);
}

void setInt(int64_t intValue) {
  setType(VT_INT);
  _intValue = intValue;
}

int64_t getInt() const {
  ensureType(VT_INT);
  return _intValue;
}

void setDouble(double doubleValue) {
  setType(VT_DOUBLE);
  _doubleValue = doubleValue;
}

double getDouble() const {
  ensureType(VT_DOUBLE);
  return _doubleValue;
}

void setString(char const * stringValue) {
  setType(VT_STRING);
  _stringValue = stringValue;
}

char const * getString() const {
  ensureType(VT_STRING);
  return _stringValue;
}

void setType(VarType type) {
  if (_type_ptr) {
    ensureType(type);
  } else { 
    _type = type;
    _type_ptr = &_type;
  }
}

VarType const * getType() const {
  return _type_ptr;
}

template<class T>
T getTyped() const {
  return T::inexistent_function;
}

private:
void ensureType(VarType t) const {
  if (!_type_ptr || t != _type) {
    throw std::runtime_error(std::string("Using value of type ") + 
      typeToName(_type) + " as " + 
      typeToName(t) + " without a cast.");
  }
}

private:
  VarType _type;
  VarType const * _type_ptr;
  union {
    double _doubleValue;
    int64_t _intValue;
    char const * _stringValue;
  };
};

template<>
double Value::getTyped<double>() const {
  return getDouble();
}

template<>
int64_t Value::getTyped<int64_t>() const {
  return getInt();
}


class FunctionContext {
public:
//added to be able to put it to standard containers
FunctionContext() 
  : _id(0), _vars(0), _bc(0), _instruction_pointer(0) {
}

FunctionContext(BytecodeFunction * function) 
  : _id(function->scopeId()), 
    _vars(function->localsNumber()),
    _bc(function->bytecode()),
    _instruction_pointer(0) {
}

void setVar(uint16_t id, double value) {
  _vars[id].setDouble(value);
}

void setVar(uint16_t id, int64_t value) {
  _vars[id].setInt(value);
}

void setVar(uint16_t id, char const * value) {
  _vars[id].setString(value);
}

int64_t getInt(uint16_t id) const {
  return _vars[id].getInt();
}

double getDouble(uint16_t id) const {
  return _vars[id].getDouble();
}

char const * getString(uint16_t id) const {
  return _vars[id].getString();
}

uint16_t id() const {
  return _id;
}

Instruction nextInstruction() {
  return _bc->getInsn(_instruction_pointer++);
}

template<class T> T nextTyped() {
  T value = _bc->getTyped<T>(_instruction_pointer);
  _instruction_pointer += sizeof(T);
  return value;
}

int64_t nextInt() {
  return nextTyped<int64_t>();
}

double nextDouble() {
  return nextTyped<double>();
}

uint16_t nextId() {
  return nextTyped<uint16_t>();
}

int16_t nextOffset() {
  return nextTyped<int16_t>();
}

bool isInstructionPointerValid() const {
  return _instruction_pointer < _bc->length();
}

void jump(int16_t offset) {
  _instruction_pointer += offset;
}

private:
  uint16_t _id;
  std::vector<Value> _vars;
  Bytecode * _bc;
  uint32_t _instruction_pointer;
};

class BytecodeInterpreter {
public:
BytecodeInterpreter(Code * code) : _code(code) {
}

void run() {
  LOG("running bytecode interpreter");
  try {
    call(0);
    interpret();
  } catch (std::runtime_error const & e) {
    _status = Status(e.what());
  }
}

Status const & getStatus() {
  return _status;
}

private:
#define BIN_OP(OP, TYPE) {                                \
  Value higher = pop();                                   \
  Value lower = pop();                                    \
  push(lower.getTyped<TYPE>() OP higher.getTyped<TYPE>());\
  break;                                                  \
}
#define I_BIN_OP(OP) BIN_OP(OP, int64_t)
#define D_BIN_OP(OP) BIN_OP(OP, double)
  void interpret() {
    while (!stop()) {
      Instruction instruction(ctx().nextInstruction());
      switch (instruction) {
        case BC_DLOAD: push(ctx().nextDouble()); break;
        case BC_ILOAD: push(ctx().nextInt()); break;
        case BC_SLOAD: push(_code->constantById(ctx().nextId()).c_str()); break;
        case BC_DLOAD0: push(0.0); break;
        case BC_ILOAD0: push((int64_t)0); break;
        case BC_DLOAD1: push(1.0); break;
        case BC_ILOAD1: push((int64_t)1); break;
        case BC_DLOADM1: push(-1.0); break;
        case BC_ILOADM1: push((int64_t)-1); break;
        case BC_DADD: D_BIN_OP(+)
        case BC_IADD: I_BIN_OP(+)
        case BC_DSUB: D_BIN_OP(-)
        case BC_ISUB: I_BIN_OP(-)
        case BC_DMUL: D_BIN_OP(*)
        case BC_IMUL: I_BIN_OP(*)
        case BC_DDIV: D_BIN_OP(/)
        case BC_IDIV: I_BIN_OP(/)
        case BC_IMOD: I_BIN_OP(%)
        //DNEG is not used
        case BC_INEG: {
          int64_t v = pop().getInt(); 
          push((int64_t)(v == 0 ? 1 : 0)); 
          break;
        }
        case BC_IAOR: I_BIN_OP(|)
        case BC_IAAND: I_BIN_OP(&)
        case BC_IAXOR: I_BIN_OP(^)
        case BC_IPRINT: std::cout << pop().getInt(); break;
        case BC_DPRINT: std::cout << pop().getDouble(); break;
        case BC_SPRINT: std::cout << pop().getString(); break;
        case BC_I2D: push((double) pop().getInt()); break;
        case BC_D2I: push((int64_t) pop().getDouble()); break;
        case BC_S2I: push(parseInt(pop().getString())); break;
        case BC_SWAP: swap(); break;
        case BC_POP: pop(); break;
        //LOAD_VAR_ are not used
        //STORE_VAR_ are not used
        case BC_LOADDVAR: push(ctx().getDouble(ctx().nextId())); break;    
        case BC_LOADIVAR: push(ctx().getInt(ctx().nextId())); break;    
        case BC_LOADSVAR: push(ctx().getString(ctx().nextId())); break;
        case BC_STOREDVAR: ctx().setVar(ctx().nextId(), pop().getDouble()); break;
        case BC_STOREIVAR: ctx().setVar(ctx().nextId(), pop().getInt()); break;
        case BC_STORESVAR: ctx().setVar(ctx().nextId(), pop().getString()); break;
        case BC_LOADCTXDVAR: push(ctx(ctx().nextId()).getDouble(ctx().nextId())); break;    
        case BC_LOADCTXIVAR: push(ctx(ctx().nextId()).getInt(ctx().nextId())); break;    
        case BC_LOADCTXSVAR: push(ctx(ctx().nextId()).getString(ctx().nextId())); break;
        case BC_STORECTXDVAR: ctx(ctx().nextId()).setVar(ctx().nextId(), pop().getDouble()); break;
        case BC_STORECTXIVAR: ctx(ctx().nextId()).setVar(ctx().nextId(), pop().getInt()); break;
        case BC_STORECTXSVAR: ctx(ctx().nextId()).setVar(ctx().nextId(), pop().getString()); break;
        case BC_DCMP: cmp<double>(); break;
        case BC_ICMP: cmp<int64_t>(); break;
        case BC_JA: jump(); break;
        case BC_IFICMPNE: jumpIf(cmp<int64_t>(false)); break;
        case BC_IFICMPE: jumpIf(!cmp<int64_t>(false)); break;
        case BC_IFICMPG: jumpIf(cmp<int64_t>(false) < 0); break;
        case BC_IFICMPGE: jumpIf(cmp<int64_t>(false) <= 0); break;
        case BC_IFICMPL: jumpIf(cmp<int64_t>(false) > 0); break;
        case BC_IFICMPLE: jumpIf(cmp<int64_t>() >= 0); break;
        //DUMP is not used
        //STOP is not used
        case BC_CALL: call(); break;
        //CALLNATIVE is not used
        case BC_RETURN: ret(); break;
        //BREAK is not used
        default: {
          throw std::runtime_error(std::string("Unknown bytecode instruction: ") + 
            bytecodeName(instruction, 0));
        }
      }
    }
  }
#undef I_BIN_OP
#undef D_BIN_OP
#undef BIN_OP
  
  FunctionContext & ctx() {
    return _contexts.back();
  }
  
  FunctionContext & ctx(uint16_t id) {
    return _contexts[id];
  }

  bool stop() {
    return _contexts.empty() || !ctx().isInstructionPointerValid();
  }

  Value pop() {
    Value top = _execution_stack.back();
    _execution_stack.pop_back();
    return top; 
  }
  
  void push(Value v) {
    _execution_stack.push_back(v);
  }
  
  void call() {
    call(ctx().nextId());
  }

  void call(uint16_t functionId) {
    _contexts.push_back(FunctionContext(dynamic_cast<BytecodeFunction *>(_code->functionById(functionId))));
  }
  
  void ret() {
    _contexts.pop_back();
  }
  
  void swap() {
    Value higher = pop();
    Value lower = pop();
    push(higher);
    push(lower);
  }

  template<class T>
  int64_t cmp(bool pushResult = true) {
    T upper = pop().getTyped<T>();
    T lower = pop().getTyped<T>();
    int64_t result = 0;
    if (lower < upper) {
      result = -1;
    } else if (lower > upper) {
      result = 1;
    } 
    if (pushResult) push(result);
    return result;
  }
  
  void jump() {
    int16_t offset = ctx().nextOffset();
    ctx().jump(offset - sizeof(offset));
  }
  
  void jumpIf(bool what) {
    if (what) {
      jump();
    } else {
      ctx().nextOffset();
    }
  }

  int64_t parseInt(char const * string) const {
    char * p;
    int64_t result = strtoll(string, &p, 10);
    if ('\0' != *p) {
      throw std::runtime_error("Sting to integer conversion failed.");
    }
    return result;
  }

private:
  std::vector<Value> _execution_stack;
  std::vector<FunctionContext> _contexts;
  Code * _code;
  Status _status;
};

Status* InterpreterCodeImpl::execute(std::vector<Var*> & vars) {
  BytecodeInterpreter i(this);
  i.run();
  return new Status(i.getStatus());
}

}
