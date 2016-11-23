#include "context.h"
#include "translator_exception.h"
#include <limits>

using namespace mathvm;


Context::Context(BytecodeFunction* function, Scope* scope, Context* parent)
    : _parent(parent), _function(function), _scope(scope), _maxId(0u) {
  Scope::VarIterator it(_scope);
  while (it.hasNext()) {
    _variableName2Id[it.next()->name()] = _maxId++;
  }
}

BytecodeFunction* Context::getCurrentFunction() const {
  return _function;
}

VarInfo Context::getVarInfo(std::string const& name) {
  if (_variableName2Id.find(name) == _variableName2Id.end()) {
    if (_parent != nullptr) {
      return _parent->getVarInfo(name);
    } else {
      throw TranslationException("Could not find variable " + name + " in all scopes", Status::INVALID_POSITION);
    }
  }

  return VarInfo{_variableName2Id[name], _function->id()};
}

void Context::addVariable(AstVar const* variable) {
  uint16_t limit = std::numeric_limits<uint16_t>::max();
  if (_variableName2Id.size() > limit) {
    throw TranslationException("Scope may contains only" + std::to_string(limit) + " variables",
                               Status::INVALID_POSITION);
  }

  _scope->declareVariable(variable->name(), variable->type());
  _variableName2Id[variable->name()] = _maxId++;
}

uint16_t Context::getId() const {
  return _function->id();
}

uint16_t Context::getLocalsCount() const {
  return static_cast<uint16_t>(_variableName2Id.size());
}

Context* Context::getParent() {
  return _parent;
}
