#ifndef SCOPE_CTX_H
#define SCOPE_CTX_H

#include "ast.h"
#include "mathvm.h"

#include <memory>

namespace mathvm
{

struct Location
{
    uint16_t m_scopeId;
    uint16_t m_varId;
};

class ScopeContext;
using ScopeContextPtr = std::shared_ptr<ScopeContext>;

class ScopeContext
{
public:
    ScopeContext(std::shared_ptr<ScopeContext> parent, AstFunction * fn);

    Bytecode * bytecode();
    Scope * scope();

    TranslatedFunction * function();
    void addVar(AstVar const * var);

    VarType returnType() const;
    bool isLocal(const std::string& varName) const;
    Location location(const std::string& varName) const;

private:
    ScopeContextPtr m_parent;
    BytecodeFunction * m_function;
    std::unique_ptr<Scope> m_scope;

    uint16_t m_varId = 0;
    std::map<std::string, uint16_t> m_varIds;
};

}   // namespace mathvm

#endif // SCOPE_CTX_H
