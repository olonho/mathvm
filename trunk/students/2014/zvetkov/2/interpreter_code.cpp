#include "interpreter_code.hpp"

using namespace mathvm;

InterpreterCodeImpl::InterpreterCodeImpl() {}

InterpreterCodeImpl::~InterpreterCodeImpl() {}

Status* InterpreterCodeImpl::execute(vector<Var*>& vars) {
  return Status::Error("Not implemented InterpreterCodeImpl::execute");
}

BytecodeFunction* InterpreterCodeImpl::functionByName(const string& name) {
  return dynamic_cast<BytecodeFunction*>(Code::functionByName(name));
}

BytecodeFunction* InterpreterCodeImpl::functionById(uint16_t id) {
  return dynamic_cast<BytecodeFunction*>(Code::functionById(id));
}