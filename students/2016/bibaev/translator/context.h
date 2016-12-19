#pragma once

#include <unordered_map>
#include <mathvm.h>
#include <ast.h>
#include <stack>

namespace mathvm {
struct VarInfo {
public:
  const uint16_t localId;
  const uint16_t contextId;

  VarInfo()
      : localId(0), contextId(0) {
  }

  VarInfo(uint16_t id, uint16_t context)
      : localId(id), contextId(context) {
  }

  VarInfo(VarInfo const& other)
      : localId(other.localId), contextId(other.contextId) {
  }
};

class Context {
public:
  Context(BytecodeFunction* function, Scope* scope, Context* parent = nullptr);

  BytecodeFunction* getCurrentFunction() const;

  void addVariable(AstVar const* variable);

  VarInfo getVarInfo(std::string const& name);

  uint16_t getId() const;

  uint16_t getLocalsCount() const;

  Context* getParent();

private:
  Context* _parent;
  BytecodeFunction* _function;
  Scope* _scope;
  uint16_t _maxId;
  std::unordered_map<std::string, uint16_t> _variableName2Id;

}; // Context
} // mathvm

