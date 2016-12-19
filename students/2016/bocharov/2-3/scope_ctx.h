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
    ScopeContext(std::shared_ptr<ScopeContext> parent, BytecodeFunction * fn);

    Bytecode * bytecode();

    void setId(uint16_t id);

    void addFunction(AstFunction * function);
    AstFunction * getFunction(std::string const & name) const;
    TranslatedFunction * function();

    void addVar(std::string const & name);
    bool isLocal(const std::string& varName) const;
    Location location(const std::string& varName) const;
    uint16_t localsNumber() const;

    VarType returnType() const;

private:
    ScopeContextPtr m_parent;
    BytecodeFunction * m_function;

    std::map<std::string, AstFunction *> m_functions;

    uint16_t m_varId = 0;
    std::map<std::string, uint16_t> m_varIds;
};

}   // namespace mathvm

#endif // SCOPE_CTX_H
