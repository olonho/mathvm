#ifndef CONTEXT_HPP
#define CONTEXT_HPP

#include "ast.h"
#include "mathvm.h"

#include "info.hpp"
#include "interpreter_code.hpp"

#include <map>
#include <stack>
#include <string>
#include <vector>
#include <utility>

namespace mathvm {

class Context {
  typedef std::map<AstFunction*, uint16_t> IdByFunctionMap;

  InterpreterCodeImpl* code_;
  std::stack<uint16_t> functionIds_;
  std::stack<Scope*> scopes_;
  std::vector<VarInfo*> varInfos_; 
  IdByFunctionMap idByFunction_;

public:
  Context(InterpreterCodeImpl* code)
    : code_(code) {}

  ~Context();

  void addFunction(AstFunction* function);
  uint16_t addNativeFunction(const string& name, const Signature& signature, const void* address);
  void enterFunction(AstFunction* function);
  void exitFunction();
  uint16_t currentFunctionId() const;

  uint16_t getId(AstFunction* function);
  Bytecode* bytecodeByFunctionId(uint16_t id);
  InterpreterFunction* currentFunction() const;
  InterpreterFunction* functionById(uint16_t id) const;

  void enterScope(Scope* scope);
  void exitScope();
  Scope* currentScope() const;

  uint16_t makeStringConstant(const std::string& string);
  uint16_t declareTemporary();
  void declare(AstVar* var);
};

} // namespace mathvm

#endif