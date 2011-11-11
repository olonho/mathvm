#pragma once
#include <ostream>
#include <ast.h>
#include <map>
#include <vector>
#include <sstream>
#include <iostream>
#include <stdlib.h>
#include <mathvm.h>
#include <ast.h>
#include <visitors.h>
#include "TranslationException.h"
#include "TranslationUtils.h"
#include "SymbolStack.h"

struct VarInfo {
  VarInfo(const std::string& name, mathvm::VarType type, size_t ownerId) 
    : name(name)
    , type(type)
    , ownerId(ownerId)
  {}
  std::string name;
  mathvm::VarType type;
  size_t ownerId; //func owner of the var
};

class TypeVisitor: public mathvm::AstVisitor {

  mathvm::AstFunction* topAstFunc;
  NodeInfos nodeInfo;
  SymbolStack<VarInfo> varInfo;
  SymbolStack<Params> funcParams;
  const char* curFuncName;
  mathvm::FunctionNode *curFuncNode;
  FunctionContexts& funcContexts;
  FunctionNodeToIndex funcNodeToIndex;
  IndexToFunctionNode indexToFuncNode;

  void typeError(std::string str = "", mathvm::AstNode* node = 0); 
  size_t newVarId();
  mathvm::VarType binNodeType(mathvm::BinaryOpNode* node, mathvm::AstNode* left, mathvm::AstNode* right);
  mathvm::VarType curTypeOfVarInContext(const std::string& varName, size_t funcContextId);
  public:
  TypeVisitor(mathvm::AstFunction* top,
              FunctionContexts& funcContexts, 
              const FunctionNodeToIndex& funcNodeToIndex, 
              const IndexToFunctionNode& indexToFuncNode);
  void  visit();
  NodeInfos& getNodeInfo() { return nodeInfo; }

#define VISITOR_FUNCTION(type, name) \
  void visit##type(mathvm::type* node);
  FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION
};
