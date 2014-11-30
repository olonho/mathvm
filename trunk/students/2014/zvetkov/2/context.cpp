#include "context.hpp"

namespace mathvm {

Context::~Context() {
  for (size_t i = 0; i < varInfos_.size(); ++i) {
    delete varInfos_[i];
  }
}

void Context::addFunction(AstFunction* function) {
  uint16_t deepness = static_cast<uint16_t>(functionIds_.size());
  uint16_t id = code_->addFunction(new InterpreterFunction(function, deepness));
  idByFunction_.insert(std::make_pair(function, id));
}

uint16_t Context::addNativeFunction(const string& name, const Signature& signature, const void* address) {
  return code_->makeNativeFunction(name, signature, address);
}

void Context::enterFunction(AstFunction* function) {
  functionIds_.push(getId(function));
}

void Context::exitFunction() {
  assert(!functionIds_.empty());
  functionIds_.pop();
}

uint16_t Context::getId(AstFunction* function) {
  IdByFunctionMap::iterator it = idByFunction_.find(function);
  assert(it != idByFunction_.end());
  return it->second;
}

uint16_t Context::currentFunctionId() const {
  assert(!functionIds_.empty());
  return functionIds_.top();
}

void Context::enterScope(Scope* scope) {
  scopes_.push(scope);
}

void Context::exitScope() {
  assert(!scopes_.empty());
  scopes_.pop();
}

Scope* Context::currentScope() const {
  assert(!scopes_.empty());
  return scopes_.top();
}

Bytecode* Context::bytecodeByFunctionId(uint16_t id) {
  BytecodeFunction* function = functionById(id);
  return (function == 0) ? 0 : function->bytecode();
}

InterpreterFunction* Context::currentFunction() const {
  return functionById(currentFunctionId());
}

InterpreterFunction* Context::functionById(uint16_t id) const {
  return code_->functionById(id);
}

uint16_t Context::makeStringConstant(const std::string& string) {
  return code_->makeStringConstant(string);
}

uint16_t Context::declareTemporary() {
  InterpreterFunction* function = currentFunction();
  uint16_t localsNumber = static_cast<uint16_t>(function->localsNumber());
  function->setLocalsNumber(localsNumber + 1);
  return localsNumber;
}

void Context::declare(AstVar* var) {
  InterpreterFunction* function = currentFunction();
  uint16_t functionId = function->id();
  uint16_t localsNumber = static_cast<uint16_t>(function->localsNumber());
  VarInfo* info = new VarInfo(functionId, localsNumber++);
  function->setLocalsNumber(localsNumber);
  var->setInfo(info);
  varInfos_.push_back(info);
}

} // namespace mathvm