#include "Scopes.h"

using namespace mathvm;

BlockScope::BlockScope(mathvm::Scope* scope, FunctionScope* parentFunction) : myParentFunction(parentFunction), myScope(scope) {
    declareVariables();
}

BlockScope::BlockScope(mathvm::Scope* scope) : myParentFunction(NULL), myScope(scope) 
{ }

bool BlockScope::varExists(std::string const & name) const {
    return myVars.find(name) != myVars.end();
}

bool BlockScope::functionExists(std::string const & name) const {
    return myFunctions.find(name) != myFunctions.end();
}

void BlockScope::declareFunction(FunctionId const & fid) {
    FunctionIdMap::iterator it = myFunctions.find(fid.function->name());
    if (it != myFunctions.end())
        throw TranslationException("Function redefinition: " + fid.function->name());
    myFunctions[fid.function->name()] = fid;
}

void BlockScope::declareVariable(std::string const & name) {
    if (varExists(name))
        throw TranslationException("Variable redefinition: " + name);
    assert(myParentFunction);
    VarId vid;
    vid.id = myParentFunction->myTotalVariablesNum++;
    vid.ownerFunction = myParentFunction->myId.id;
    myVars[name] = vid;
}

VarId BlockScope::getVariableId(std::string const & name) {
    VarIdMap::iterator it = myVars.find(name);
    if (it == myVars.end())
        throw TranslationException("Undefined variable: " + name);
    return it->second;
}

void BlockScope::declareVariables() {
    assert(myScope);
    Scope::VarIterator it(myScope);
    while (it.hasNext()) {
        AstVar * var = it.next();
        declareVariable(var->name());
    }
}

//------------------------------------------
FunctionScope::FunctionScope(FunctionId const & id, mathvm::Scope * bodyScope) : BlockScope(bodyScope), myTotalVariablesNum(0), myId(id) {
    myParentFunction = this;
    for (uint32_t i = 0; i < myId.function->parametersNumber(); ++i) {
        declareVariable(myId.function->parameterName(i));
    }
    declareVariables();
}

mathvm::AstFunction* FunctionScope::getAstFunction() const {
    return myId.function;
}
