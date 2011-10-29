#include "GeneratorCommon.h"

using namespace mathvm;


mathvm::AstFunction* ScopeInfo::GetAstFunction()
{
  return functionId.function;
}

uint16_t ScopeInfo::GetTotalVariablesNum()
{
  return totalVariablesNum;
}

mathvm::Scope* ScopeInfo::GetScope() const
{
  return scope;
}

bool ScopeInfo::IsFunction()
{
  return isFunction;
}

uint16_t ScopeInfo::GetFunctionId()
{
  return functionId.id;
}

uint16_t ScopeInfo::GetInitialIndex() const
{
  return variableCounter;
}

bool ScopeInfo::TryFindVariableId( std::string const & name, uint16_t & outId )
{
  std::map<std::string, uint16_t>::iterator it = myVars.find(name);
  if (it == myVars.end()) return false;
  outId = it->second;
  return true;
}