#ifndef GENERATOR_CONTEXT_HANDLER_H
#define GENERATOR_CONTEXT_HANDLER_H
#include <map>
#include "ast.h"
#include "mathvm.h"

namespace mathvm {

struct VarInfo {
    uint16_t contextId;
    uint16_t localId;
    bool isLocal;
};

class ScopeHandler {
    Scope *m_scope;
    ScopeHandler *m_parent;
    std::map<string, uint16_t> m_varNameToId;
public:
    ScopeHandler(uint16_t iniLocalId, Scope *scope, ScopeHandler *parent = 0);
    ScopeHandler *parent() { return m_parent; }
    bool containsVar(const std::string& varName, bool useParent = true);
    uint16_t getVarId(const std::string& varName);
    uint32_t getLocalsNum() { return m_scope->variablesCount(); }
};

class ContextHandler {
    uint16_t m_id;
    uint16_t m_localIdCnt;
    uint16_t m_localRequaredNum;

    ContextHandler *m_parent;
    ScopeHandler *m_currentScopeHandler;
    AstFunction *m_currentFunction;
    VarType m_tosOperandType;
    Bytecode *m_bytecode;

public:
    ContextHandler(AstFunction *currentFunction, Bytecode *bytecode, uint16_t id, ContextHandler *parent = 0);
    bool pushScope(Scope *scope);
    void popScope();

    Bytecode *bytecode() { return m_bytecode; }
    bool containsVar(const std::string& varName) { return m_currentScopeHandler->containsVar(varName); }
    ContextHandler *parent() { return m_parent; }
    VarInfo getVarInfo(const std::string& varName);
    uint16_t getContextSize() { return m_localRequaredNum; }
    AstFunction *currentFunction() { return m_currentFunction; }
    void setTosOperandType(VarType type) { m_tosOperandType = type; }
    VarType tosOperandType() { return m_tosOperandType; }
    bool isTosOperandType(VarType type) { return m_tosOperandType == type; }
};

}

#endif // GENERATOR_CONTEXT_HANDLER_H
