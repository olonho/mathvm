#include "GeneratorCommon.h"

using namespace mathvm;

bool VariableScopeManager::IsVarOnStack( mathvm::AstVar const * var ) const
{
  StackType::const_reverse_iterator it = myScopeStack.crbegin();
  for (; it != myScopeStack.crend(); ++it) {
    if ((*it)->VarExists(var->name())) return true;
  }
  return false;
}

bool VariableScopeManager::IsFunctionVisible( std::string const& functionName ) const
{
  StackType::const_reverse_iterator it = myScopeStack.crbegin();
  for (; it != myScopeStack.crend(); ++it) {
    if ((*it)->FunctionExists(functionName)) return true;
  }
  return false;
}

void VariableScopeManager::PopScope()
{
  myScopeStack.pop_back();
}




void VariableScopeManager::PushScope( mathvm::Scope* scope )
{
  ScopeList::iterator it = myScopes.begin();
  for (; it != myScopes.end(); ++it) {
    if ((*it)->Scope() == scope) {
      myScopeStack.push_back(*it);
      return;
    }
  }
}

VarId VariableScopeManager::GetVariableId( mathvm::AstVar const* var )
{
  StackType::const_reverse_iterator it = myScopeStack.crbegin();
  for (; it != myScopeStack.crend(); ++it) {
    if ((*it)->VarExists(var->name())) {
      VarId vid = (*it)->GetVariableId(var->name());
      return vid;
    }
  }
  throw TranslationException("ERROR: something bad happened");
}

uint16_t VariableScopeManager::GetFunctionId( std::string const & functionName )
{
  return myFunctionsDeclarations[functionName].id;
}

bool VariableScopeManager::IsClosure( std::string const & name )
{
  for (unsigned int i = myScopeStack.size(); i > 0; --i) {
    BlockScope * scope = myScopeStack[i-1];
    if (scope->VarExists(name)) return false;
    if (scope->IsFunction()) return true;
  }
  return false;
}

void VariableScopeManager::AddTopFunction( mathvm::AstFunction * main )
{
  assert(myFunctionsDeclarations.empty());
  FunctionID fid(main, 0);
  myFunctionsDeclarations[main->name()] = fid;
}

VariableScopeManager::VariableScopeManager() : myFunctionAutoIncrement(1), myCurrentFunctionScope(NULL)
{

}

void VariableScopeManager::CreateFunctionScope( mathvm::FunctionNode* node, mathvm::Scope *scope )
{
  FunctionMap::iterator it = myFunctionsDeclarations.find(node->name());
  if (it == myFunctionsDeclarations.end()) throw TranslationException("Undefined function: " + node->name());

  FunctionScope * fscope = new FunctionScope(it->second, scope);
  mathvm::Scope::FunctionIterator fit (scope);
  while (fit.hasNext()) {
    mathvm::AstFunction* fun = fit.next(); 
    FunctionID fid(fun, myFunctionAutoIncrement++);
    fscope->DeclareFunction(fid);  
    myFunctionsDeclarations[fun->name()] = fid;
  }

  myScopes.push_back(fscope);
  myScopeStack.push_back(fscope);
  myCurrentFunctionScope = fscope;
  myFunctionScopes[node->name()] = fscope;
}

void VariableScopeManager::CreateBlockScope( mathvm::Scope* scope )
{
  BlockScope * bscope = new BlockScope(scope, myCurrentFunctionScope);
  mathvm::Scope::FunctionIterator fit (scope);
  while (fit.hasNext()) {
    mathvm::AstFunction* fun = fit.next(); 
    FunctionID fid(fun, myFunctionAutoIncrement++);
    bscope->DeclareFunction(fid);  
    myFunctionsDeclarations[fun->name()] = fid;
  }

  myScopes.push_back(bscope);
  myScopeStack.push_back(bscope);
}

FunctionScope const * VariableScopeManager::GetFunctionScope( std::string const & name )
{
  FunctionScopeMap::iterator it = myFunctionScopes.find(name);
  if (it == myFunctionScopes.end()) throw TranslationException("Undefined function: " + name);
  return it->second;
}

void VariableScopeManager::Cleanup()
{
  for (uint32_t i = 0; i < myScopes.size(); ++i) {
    delete myScopes[i];
  }
}

mathvm::VarType VariableScopeManager::GetFunctionReturnType( std::string const & name )
{
  FunctionMap::iterator it = myFunctionsDeclarations.find(name);
  if (it == myFunctionsDeclarations.end()) throw TranslationException("Undefined function: " + name);
  return it->second.function->returnType();
}

mathvm::AstFunction const * VariableScopeManager::GetFunctionDeclaration( std::string const & name )
{
  FunctionMap::iterator it = myFunctionsDeclarations.find(name);
  if (it == myFunctionsDeclarations.end()) throw TranslationException("Undefined function: " + name);
  return it->second.function;
}



BlockScope::BlockScope( mathvm::Scope* scope, FunctionScope* parentFunction ) : myScope(scope), myParentFunction(parentFunction)
{
  DeclareVariables();
}

BlockScope::BlockScope(mathvm::Scope* scope) : myScope(scope), myParentFunction(NULL)
{

}

bool BlockScope::VarExists( std::string const & name ) const
{
  return myVars.find(name) != myVars.end();
}

bool BlockScope::FunctionExists( std::string const & name ) const
{
  return myFunctions.find(name) != myFunctions.end();
}

void BlockScope::DeclareFunction( FunctionID const & fid )
{
  FunctionIdMap::iterator it = myFunctions.find(fid.function->name());
  if (it != myFunctions.end()) throw TranslationException("Function redefinition: " + fid.function->name());
  myFunctions[fid.function->name()] = fid;
}

void BlockScope::DeclareVariable( std::string const & name)
{
  if (VarExists(name)) throw TranslationException("Variable redefinition: " + name);
  
  assert(myParentFunction);

  VarId vid;
  vid.id = myParentFunction->myTotalVariablesNum++;
  vid.ownerFunction = myParentFunction->myId.id;
  myVars[name] = vid;
}


VarId BlockScope::GetVariableId( std::string const & name )
{
  VarIdMap::iterator it = myVars.find(name);
  if (it == myVars.end()) throw TranslationException("Undefined variable: " + name);
  return it->second;
}

void BlockScope::DeclareVariables()
{
  assert(myScope);
  Scope::VarIterator it(myScope);
  while (it.hasNext()) {
    AstVar * var = it.next();
    DeclareVariable(var->name());
  }
}

FunctionScope::FunctionScope( FunctionID const & id, mathvm::Scope * bodyScope ) : myId(id), myTotalVariablesNum(0), BlockScope(bodyScope)
{
  myParentFunction = this;

  for (uint32_t i = 0; i < myId.function->parametersNumber(); ++i) {
    DeclareVariable(myId.function->parameterName(i));
  }

  DeclareVariables();
}

mathvm::AstFunction* FunctionScope::GetAstFunction() const
{
  return myId.function;
}
