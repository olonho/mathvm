#pragma once

#include "mathvm.h"
#include "ast.h"
#include <deque>
#include <vector>
#include <list>
#include <stdint.h>
#include <set>
#include <cstdarg>
#include <stdio.h>


struct TranslationException {
  TranslationException(std::string const& message) : myMessage(message), myNode(NULL) {
  }

  TranslationException(mathvm::AstNode * where, char const * message, ...) : myNode(where){
    char buf[512];
    va_list argptr; 
    va_start(argptr, message);
    vsprintf(buf, message, argptr);
    va_end(argptr); 
    myMessage = buf;
  }
  virtual std::string what() const {
    return myMessage;
  }
  virtual mathvm::AstNode* where() const {
    return myNode;
  }
private:
  std::string myMessage;
  mathvm::AstNode* myNode;
};

struct VarId {
  uint16_t id;
  uint16_t ownerFunction;
};

struct FunctionID {
  mathvm::AstFunction* function;
  uint16_t id;

  FunctionID(mathvm::AstFunction* function, uint16_t id) : function(function), id(id) {}
  FunctionID() : function(NULL), id(0) {}
};


struct ScopeInfo {

  ScopeInfo * GetParent() const {return parent;}
  mathvm::VarType GetFunctionReturnType() const {return functionId.function->returnType();}
  void UpdateTotalVars() {
    if (parent == NULL) return;
    parent->totalVariablesNum = std::max<uint16_t>(parent->totalVariablesNum, parent->scope->variablesCount() + totalVariablesNum);
  }

  ScopeInfo(): parent(NULL), scope(NULL), variableCounter(0), totalVariablesNum(0), isFunction(true)  {}

  ScopeInfo(mathvm::Scope * scope, ScopeInfo * prevScopeInfo) : parent(prevScopeInfo), scope(scope), variableCounter(0),
    totalVariablesNum(scope->variablesCount()), isFunction(false)
  {
    if (prevScopeInfo) {
      functionId = prevScopeInfo->functionId;
      variableCounter = prevScopeInfo->variableCounter + prevScopeInfo->scope->variablesCount();
    }
    FillVars(scope);
  }

  ScopeInfo(mathvm::Scope * scope, ScopeInfo * prevScopeInfo, FunctionID const & functionId) : parent(prevScopeInfo), scope(scope), functionId(functionId), 
    variableCounter(functionId.function->parametersNumber()), totalVariablesNum(scope->variablesCount() + functionId.function->parametersNumber()), isFunction(true)
  {
    FillVars(scope);
  }
  mathvm::AstFunction* GetAstFunction();
  uint16_t GetTotalVariablesNum();
  mathvm::Scope* GetScope() const;
  bool IsFunction();
  uint16_t GetFunctionId();
  uint16_t GetInitialIndex() const;
  bool TryFindVariableId(std::string const & name, uint16_t & outId);
private:
  ScopeInfo * parent;
  mathvm::Scope * scope;
  FunctionID functionId;
  uint16_t variableCounter;
  uint16_t totalVariablesNum;
  bool isFunction;
  std::map<std::string, uint16_t> myVars;
  void FillVars(mathvm::Scope* scope) {
    if (isFunction) {
      for (unsigned int i = 0; i < functionId.function->parametersNumber(); ++i) {
        myVars[functionId.function->parameterName(i)] = i;
      }
    }

    if ((int) variableCounter + scope->variablesCount() > (uint16_t)-1) throw TranslationException("Max variables number exceeded");

    mathvm::Scope::VarIterator it (scope);
    for (uint16_t i = variableCounter; it.hasNext(); ++i) {
      mathvm::AstVar* var = it.next();
      myVars[var->name()] = i;
    }
  }
};

struct NodeInfo {
  mathvm::VarType nodeType;
  ScopeInfo * scopeInfo;

  NodeInfo (mathvm::VarType type, ScopeInfo * scopeInfo) : nodeType(type), scopeInfo(scopeInfo) {}
};
