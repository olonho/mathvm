#include "scope_ctx.h"
#include "bytecode_generator.h"

namespace mathvm
{

ScopeContext::ScopeContext(std::shared_ptr<ScopeContext> parent, BytecodeFunction * fn)
    : m_parent(parent)
    , m_function(fn)
{}

Bytecode * ScopeContext::bytecode()
{
    if (m_function)
        return m_function->bytecode();
    return m_parent->bytecode();
}

void ScopeContext::setId(uint16_t id)
{
    m_function->setScopeId(id);
}


void ScopeContext::addFunction(AstFunction * function)
{
    auto name = function->name();
    auto it = m_functions.find(name);
    if (it != m_functions.end()) {
        auto message = std::string("redefenition of function " + name);
        throw CodeGenerationError(message, Status::INVALID_POSITION);
    }

    m_functions[name] = function;
}

AstFunction * ScopeContext::getFunction(std::string const & name) const
{
    auto it = m_functions.find(name);
    if (it != m_functions.end()) {
        return it->second;
    }
    return m_parent ? m_parent->getFunction(name) : nullptr;
}


TranslatedFunction * ScopeContext::function()
{
    return m_function;
}

uint16_t ScopeContext::localsNumber() const
{
    return m_varIds.size();
}

bool ScopeContext::isLocal(const std::string& varName) const
{
    return m_varIds.find(varName) != m_varIds.end();
}

Location ScopeContext::location(const std::string& varName) const
{
    if (!isLocal(varName))
        return m_parent ? m_parent->location(varName) : Location{ INVALID_ID, INVALID_ID };
    auto varId = m_varIds.find(varName)->second;
    return { m_function->scopeId(), varId };
}

void ScopeContext::addVar(std::string const & name)
{
    m_varIds[name] = m_varId++;
}

VarType ScopeContext::returnType() const
{
    if (m_function)
        return m_function->returnType();
    return m_parent->returnType();
}

}   // namespace mathvm
