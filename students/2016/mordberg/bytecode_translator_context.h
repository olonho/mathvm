#pragma once

#include "mathvm.h"
#include "ast.h"

#include <memory>
#include <map>
#include <string>
#include <stdexcept>

namespace mathvm {

class ContextLookupException: public std::logic_error {
public:
    ContextLookupException(const std::string& msg, uint32_t position)
        : std::logic_error(msg)
        , _position(position) {}

    uint32_t get_position() const {
        return _position;
    }

private:
    uint32_t _position;
};

class BytecodeTranslatorContext {
public:
    BytecodeTranslatorContext(BytecodeTranslatorContext* parent, BytecodeFunction* bf, Scope* scope);

    BytecodeFunction* get_bytecode_function() const;
    uint16_t get_context_id(const std::string& var_name, uint32_t position) const;
    uint16_t get_var_id(const std::string& var_name, uint32_t position) const;
    uint16_t get_id() const;
    BytecodeTranslatorContext* get_parent() const;
    uint16_t get_locals_count() const;
    void add_var(AstVar* var, uint32_t position);
private:
    BytecodeTranslatorContext*      _parent;
    BytecodeFunction*               _bf;
    Scope*                          _scope;
    uint16_t                        _free_id;
    std::map<std::string, uint16_t> _var_resolver;
};

}