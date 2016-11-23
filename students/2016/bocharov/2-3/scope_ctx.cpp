#include "scope_ctx.h"

namespace mathvm
{

ScopeContext::ScopeContext(std::shared_ptr<ScopeContext> parent, AstFunction * fn)
    : m_parent(parent)
    , m_function(fn ? new BytecodeFunction(fn) : nullptr)
    , m_scope(new Scope(parent ? parent->scope() : nullptr))
{
    assert(fn && "function of scope context can't be null");
}

Bytecode * ScopeContext::bytecode()
{
    if (m_function)
        return m_function->bytecode();
    return m_parent->bytecode();
}

Scope * ScopeContext::scope()
{
    return m_scope.get();
}

TranslatedFunction * ScopeContext::function()
{
    return m_function;
}


VarType ScopeContext::returnType() const
{
    if (m_function)
        return m_function->returnType();
    return m_parent->returnType();
}

bool ScopeContext::isLocal(const std::string& varName) const
{
    return m_scope->lookupVariable(varName, false);
}

Location ScopeContext::location(const std::string& varName) const
{
    if (!isLocal(varName))
        return m_parent ? Location{ INVALID_ID, INVALID_ID } : m_parent->location(varName);
    auto varId = m_varIds.find(varName)->second;
    return { m_function->id(), varId };
}

void ScopeContext::addVar(AstVar const * var)
{
    m_scope->declareVariable(var->name(), var->type());
    m_varIds[var->name()] = m_varId++;
}


}   // namespace mathvm
