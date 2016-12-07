#include "bytecode_translator_context.h"

#include <limits>

namespace mathvm {

BytecodeTranslatorContext::BytecodeTranslatorContext(BytecodeTranslatorContext* parent, BytecodeFunction* bf, Scope* scope)
    : _parent(parent)
    , _bf(bf)
    , _scope(scope)
    , _free_id(0) {
        Scope::VarIterator varIt(_scope);
        while (varIt.hasNext()) {
            auto var = varIt.next();
            _var_resolver[var->name()] = _free_id++;
        }
    }

BytecodeFunction* BytecodeTranslatorContext::get_bytecode_function() const {
    return _bf;
}

uint16_t BytecodeTranslatorContext::get_id() const {
    return _bf->id();
}

BytecodeTranslatorContext* BytecodeTranslatorContext::get_parent() const {
    return _parent;
}

uint16_t BytecodeTranslatorContext::get_context_id(const std::string& var_name, uint32_t position) const {
    if (_var_resolver.count(var_name)) {
        return _bf->id();
    }
    if (_parent) {
        return _parent->get_context_id(var_name, position);
    }
    throw ContextLookupException("Undefined variable", position);
}

uint16_t BytecodeTranslatorContext::get_var_id(const std::string& var_name, uint32_t position) const {
    if (_var_resolver.count(var_name)) {
        return _var_resolver.at(var_name);
    }
    if (_parent) {
        return _parent->get_var_id(var_name, position);
    }
    throw ContextLookupException("Undefined variable", position);
}

uint16_t BytecodeTranslatorContext::get_locals_count() const {
    return (uint16_t) _var_resolver.size();
}

void BytecodeTranslatorContext::add_var(AstVar* variable, uint32_t position) {
    if (_var_resolver.size() >= std::numeric_limits<uint16_t>::max()) {
        throw ContextLookupException("Too many locals in scope", position);
    }

  _scope->declareVariable(variable->name(), variable->type());
  _var_resolver[variable->name()] = _free_id++;
}

}