#pragma once

#include "mathvm.h"
#include "ast.h"
#include <deque>
#include <vector>
#include <list>
#include <stdint.h>
#include <set>

struct TranslationException {
  TranslationException(std::string const& message) : myMessage(message) {
  }
  virtual std::string what() const {
    return myMessage;
  }
private:
  std::string myMessage;
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

struct FunctionScope;



struct BlockScope {
  BlockScope(mathvm::Scope* scope, FunctionScope* parentFunction);
  BlockScope(mathvm::Scope* scope);
  virtual ~BlockScope(){}
  bool VarExists(std::string const & name) const;
  bool FunctionExists(std::string const & name) const;
  void DeclareVariable( std::string const & name);
  void DeclareFunction( FunctionID const & fid );
  VarId GetVariableId(std::string const & name);
  mathvm::Scope const * Scope() const {return myScope;}
  virtual bool IsFunction() const {return false;}

  void DeclareVariables();
protected:
  FunctionScope * myParentFunction;
  mathvm::Scope * myScope;

  typedef std::map<std::string, VarId> VarIdMap;
  typedef std::map<std::string, FunctionID> FunctionIdMap;
  VarIdMap myVars;
  FunctionIdMap myFunctions;
};

struct FunctionScope : BlockScope {
  friend struct BlockScope;
  FunctionScope(FunctionID const & id, mathvm::Scope * bodyScope);
  uint16_t GetId() const {return myId.id;}
  mathvm::AstFunction* GetAstFunction() const;
  uint16_t GetTotalVariablesNum() const {return myTotalVariablesNum;}
  virtual bool IsFunction() const {return true;}
private:
  uint16_t myTotalVariablesNum;
  FunctionID myId;
};

struct VariableScopeManager {
  VariableScopeManager();
  void Cleanup();
  bool IsVarOnStack(mathvm::AstVar const * var) const;
  bool IsFunctionVisible(std::string const& functionName) const;
  void CreateBlockScope(mathvm::Scope* scope);
  void PopScope();
  FunctionScope const * GetFunctionScope(std::string const & name);
  void PushScope( mathvm::Scope* scope );
  uint16_t GetFunctionId( std::string const & functionName );
  VarId GetVariableId(mathvm::AstVar const* var);
  bool IsClosure( std::string const & name );
  void AddTopFunction( mathvm::AstFunction * main );
  void CreateFunctionScope( mathvm::FunctionNode* node, mathvm::Scope *scope );
  mathvm::VarType GetFunctionReturnType(std::string const & name);
  mathvm::AstFunction const * GetFunctionDeclaration(std::string const & name);

private:
  typedef std::vector<BlockScope*> ScopeList;
  typedef std::deque<BlockScope*> StackType;
  typedef std::map<std::string, FunctionID> FunctionMap;
  typedef std::map<std::string, FunctionScope*> FunctionScopeMap;
  
  ScopeList myScopes;
  StackType myScopeStack;
  FunctionMap myFunctionsDeclarations;
  FunctionScopeMap myFunctionScopes;
  
  uint16_t myFunctionAutoIncrement;
  FunctionScope * myCurrentFunctionScope;
};

struct ExtendedBytecodeFunction : mathvm::BytecodeFunction {
  ExtendedBytecodeFunction(FunctionScope const * functionScope) : mathvm::BytecodeFunction(functionScope->GetAstFunction()), myVariablesNum(functionScope->GetTotalVariablesNum()) {
    mathvm::AstFunction* fun = functionScope->GetAstFunction();
    for (uint32_t i = 0; i < fun->parametersNumber(); ++i) {
      myArgumentTypes.push_back(fun->parameterType(i));
    }
  }
  uint16_t GetVariablesNum() const {return myVariablesNum;}
  std::vector<mathvm::VarType> const & GetArgumentTypes() const {return myArgumentTypes;}
private:
  uint16_t myVariablesNum;
  std::vector<mathvm::VarType> myArgumentTypes;
};
