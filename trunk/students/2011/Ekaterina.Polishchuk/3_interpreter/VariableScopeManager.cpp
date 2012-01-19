#include "VariableScopeManager.h"

using namespace mathvm;

bool VariableScopeManager::isVarOnStack(mathvm::AstVar const * var) const {
    StackType::const_reverse_iterator it = myScopeStack.rbegin();
    for (; it != myScopeStack.rend(); ++it) {
        if ((*it)->varExists(var->name()))
            return true;
    }
    return false;
}

bool VariableScopeManager::isFunctionVisible(std::string const& functionName) const {
    StackType::const_reverse_iterator it = myScopeStack.rbegin();
    for (; it != myScopeStack.rend(); ++it) {
        if ((*it)->functionExists(functionName))
            return true;
    }
    return false;
}

void VariableScopeManager::popScope() {
    myScopeStack.pop_back();
}

void VariableScopeManager::pushScope(mathvm::Scope* scope) {
    ScopeList::iterator it = myScopes.begin();
    for (; it != myScopes.end(); ++it) {
        if ((*it)->scope() == scope) {
            myScopeStack.push_back(*it);
            return;
        }
    }
}

VarId VariableScopeManager::getVariableId(mathvm::AstVar const* var) {
    StackType::const_reverse_iterator it = myScopeStack.rbegin();
    for (; it != myScopeStack.rend(); ++it) {
        if ((*it)->varExists(var->name())) {
            VarId vid = (*it)->getVariableId(var->name());
            return vid;
        }
    }
    throw TranslationException("ERROR: getVariableId");
}

uint16_t VariableScopeManager::getFunctionId(std::string const & functionName) {
    return myFunctionsDeclarations[functionName].id;
}

bool VariableScopeManager::isClosure(std::string const & name) {
    for (unsigned int i = myScopeStack.size(); i > 0; --i) {
        BlockScope * scope = myScopeStack[i-1];
        if (scope->varExists(name))
            return false;
        if (scope->isFunction())
            return true;
    }
    return false;
}

void VariableScopeManager::addTopFunction(mathvm::AstFunction * main) {
    assert(myFunctionsDeclarations.empty());
    FunctionId fid(main, 0);
    myFunctionsDeclarations[main->name()] = fid;
}

VariableScopeManager::VariableScopeManager() : myFunctionAutoIncrement(1), myCurrentFunctionScope(NULL)
{ }

void VariableScopeManager::createFunctionScope(mathvm::FunctionNode* node, mathvm::Scope *scope) {
    FunctionMap::iterator it = myFunctionsDeclarations.find(node->name());
    if (it == myFunctionsDeclarations.end())
        throw TranslationException("Undefined function: " + node->name());
    FunctionScope * fscope = new FunctionScope(it->second, scope);
    mathvm::Scope::FunctionIterator fit (scope);
    while (fit.hasNext()) {
        mathvm::AstFunction* fun = fit.next();
        FunctionId fid(fun, myFunctionAutoIncrement++);
        fscope->declareFunction(fid);
        myFunctionsDeclarations[fun->name()] = fid;
    }
    myScopes.push_back(fscope);
    myScopeStack.push_back(fscope);
    myCurrentFunctionScope = fscope;
    myFunctionScopes[node->name()] = fscope;
}

void VariableScopeManager::createBlockScope(mathvm::Scope* scope) {
    BlockScope * bscope = new BlockScope(scope, myCurrentFunctionScope);
    mathvm::Scope::FunctionIterator fit (scope);
    while (fit.hasNext()) {
        mathvm::AstFunction* fun = fit.next();
        FunctionId fid(fun, myFunctionAutoIncrement++);
        bscope->declareFunction(fid);
        myFunctionsDeclarations[fun->name()] = fid;
    }
    myScopes.push_back(bscope);
    myScopeStack.push_back(bscope);
}

FunctionScope const * VariableScopeManager::getFunctionScope(std::string const & name) {
    FunctionScopeMap::iterator it = myFunctionScopes.find(name);
    if (it == myFunctionScopes.end())
        throw TranslationException("Undefined function: " + name);
    return it->second;
}

void VariableScopeManager::cleanUp() {
    for (uint32_t i = 0; i < myScopes.size(); ++i) {
        delete myScopes[i];
    }
}

mathvm::VarType VariableScopeManager::getFunctionReturnType(std::string const & name) {
    FunctionMap::iterator it = myFunctionsDeclarations.find(name);
    if (it == myFunctionsDeclarations.end())
        throw TranslationException("Undefined function: " + name);
    return it->second.function->returnType();
}

mathvm::AstFunction const * VariableScopeManager::getFunctionDeclaration(std::string const & name) {
    FunctionMap::iterator it = myFunctionsDeclarations.find(name);
    if (it == myFunctionsDeclarations.end())
        throw TranslationException("Undefined function: " + name);
    return it->second.function;
}
