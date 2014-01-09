#include <stdexcept>

#include "InterpreterCodeImpl.h"
#include "DebugMacros.h"

namespace mathvm {

class Value {
public:
Value() : _type(VT_INVALID), _type_ptr(0) {
}

void setInt(int64_t intValue) {
  _intValue = intValue;
}

uint64_t getInt() const {
  return _intValue;
}

void setDouble(double doubleValue) {
  _doubleValue = doubleValue;
}

double getDouble() const {
  return _doubleValue;
}

void setString(char const * stringValue) {
  _stringValue = stringValue;
}

char const * getString() const {
  return _stringValue;
}

void setType(VarType type) {
  if (_type_ptr) {
    throw std::runtime_error("Value type cannot be changed");
  }
  _type = type;
  _type_ptr = &_type;
}

VarType const * getType() const {
  return _type_ptr;
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

class BytecodeInterpreter {
public:
BytecodeInterpreter(Code * code) : _code(code) {
}

void run() {
  LOG("running bytecode interpreter");
  //TODO implement
}

Status const & getStatus() {
  return _status;
}

private:
  Code * _code;
  Status _status;
};

Status* InterpreterCodeImpl::execute(std::vector<Var*> & vars) {
  BytecodeInterpreter i(this);
  i.run();
  return new Status(i.getStatus());
}

}
