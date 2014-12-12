#include "generator_context_handler.h"
#include <limits>

namespace mathvm {

ScopeHandler::ScopeHandler(uint16_t iniLocalId, Scope *scope, ScopeHandler *parent)
    : m_scope(scope)
    , m_parent(parent)
{
    Scope::VarIterator it = Scope::VarIterator(m_scope);
    while(it.hasNext()) {
        AstVar *var = it.next();
        m_varNameToId[var->name()] = iniLocalId;
        iniLocalId++;
    }
}

bool ScopeHandler::containsVar(const std::string &varName, bool useParent)
{
    if (useParent) {
        if (containsVar(varName, false))
            return true;
        else if (m_parent)
            return m_parent->containsVar(varName);
        return false;
    } else {
        return m_varNameToId.count(varName);
    }
}

uint16_t ScopeHandler::getVarId(const std::string &varName)
{
    ScopeHandler *handler = this;
    while (!handler->containsVar(varName, false)) {
        handler = handler->parent();
    }
    if (handler == this) {
        return m_varNameToId[varName];
    }
    return handler->getVarId(varName);
}

ContextHandler::ContextHandler(AstFunction *currentFunction, Bytecode *bytecode, uint16_t id, ContextHandler *parent)
    : m_id(id)
    , m_localIdCnt(0)
    , m_localRequaredNum(0)
    , m_parent(parent)
    , m_currentScopeHandler(0)
    , m_currentFunction(currentFunction)
    , m_bytecode(bytecode)
{
}


VarInfo ContextHandler::getVarInfo(const std::string &varName)
{
    ContextHandler *contextHandler = this;
    while (!contextHandler->containsVar(varName)) {
        contextHandler = contextHandler->parent();
    }
    if (contextHandler == this) {
        VarInfo varInfo;
        varInfo.contextId = m_id;
        varInfo.localId = m_currentScopeHandler->getVarId(varName);
        varInfo.isLocal = true;
        return varInfo;
    }
    VarInfo varInfo = contextHandler->getVarInfo(varName);
    varInfo.isLocal = false;
    return varInfo;
}

bool ContextHandler::pushScope(Scope *scope)
{
    ScopeHandler *scopeHandler = new ScopeHandler(m_localIdCnt, scope, m_currentScopeHandler);
    uint32_t localsNum = scopeHandler->getLocalsNum();
    if (localsNum > std::numeric_limits<uint16_t>::max())
        return false;
    m_localIdCnt += localsNum;
    m_localRequaredNum = std::max(m_localRequaredNum, m_localIdCnt);
    m_currentScopeHandler = scopeHandler;
    return true;
}

void ContextHandler::popScope()
{
    assert(m_currentScopeHandler);
    ScopeHandler *scopeHandler = m_currentScopeHandler;
    m_localIdCnt -= scopeHandler->getLocalsNum();
    m_currentScopeHandler = scopeHandler->parent();
    delete scopeHandler;
}

}
