#ifndef VARIABLESCOPEMANAGER_H
#define VARIABLESCOPEMANAGER_H

#include <deque>
#include "FunctionId.h"
#include "TranslationException.h"
#include "Scopes.h"

class VariableScopeManager {
private:
    typedef std::vector<BlockScope*> ScopeList;
    typedef std::deque<BlockScope*> StackType;
    typedef std::map<std::string, FunctionId> FunctionMap;
    typedef std::map<std::string, FunctionScope*> FunctionScopeMap;

    ScopeList myScopes;
    StackType myScopeStack;
    FunctionMap myFunctionsDeclarations;
    FunctionScopeMap myFunctionScopes;

    uint16_t myFunctionAutoIncrement;
    FunctionScope * myCurrentFunctionScope;
public:	
    VariableScopeManager();
    void cleanUp();
    bool isVarOnStack(mathvm::AstVar const * var) const;
    bool isFunctionVisible(std::string const& functionName) const;
    void createBlockScope(mathvm::Scope* scope);
    void popScope();
    FunctionScope const * getFunctionScope(std::string const & name);
    void pushScope(mathvm::Scope* scope);
    uint16_t getFunctionId(std::string const & functionName);
    VarId getVariableId(mathvm::AstVar const* var);
    bool isClosure(std::string const & name);
    void addTopFunction(mathvm::AstFunction * main);
    void createFunctionScope(mathvm::FunctionNode* node, mathvm::Scope *scope);
    mathvm::VarType getFunctionReturnType(std::string const & name);
    mathvm::AstFunction const * getFunctionDeclaration(std::string const & name);
};
#endif
