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
  VarInfo(const std::string& name, mathvm::VarType type) 
    : name(name)
    , type(type)
    , index(0)
  {}
  std::string name;
  mathvm::VarType type;
  size_t index;
};

class TypeVisitor: public mathvm::AstVisitor {

  mathvm::AstFunction* topAstFunc;
  NodeInfos nodeInfo;
  SymbolStack<VarInfo> varInfo;
  SymbolStack<Params> funcParams;
  const char* curFuncName;

  void typeError(std::string str = "", mathvm::AstNode* node = 0); 
  size_t newVarId();
  mathvm::VarType  binNodeType(mathvm::BinaryOpNode* node, mathvm::AstNode* left, mathvm::AstNode* right);

  public:
  TypeVisitor(mathvm::AstFunction* top);
  void  visit();
  NodeInfos& getNodeInfo() { return nodeInfo; }

#define VISITOR_FUNCTION(type, name) \
  void visit##type(mathvm::type* node);
  FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION
};
