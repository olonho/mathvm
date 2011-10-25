#pragma once
#include <ostream>
#include <ast.h>
#include <map>
#include <vector>
#include <sstream>
#include <iostream>
#include <stdlib.h>
#include <mathvm.h>
#include <visitors.h>
#include "MyCode.h"
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

struct ParamInfo {
  std::string name;
  mathvm::VarType type;
  size_t index;
};

class TypeVisitor: public mathvm::AstVisitor {

  mathvm::AstFunction* topAstFunc;
  NodeInfos nodeInfo;
  SymbolStack<VarInfo> varInfo;
  SymbolStack<Params> funcParams;
  size_t curFuncId;

  void typeError(std::string str = ""); 
  size_t newVarId();
  void procBinNode(const NodeInfo &a, const NodeInfo &b, mathvm::TokenKind op, mathvm::VarType& resType);

  public:
  TypeVisitor(mathvm::AstFunction* top);
  void  visit();
  NodeInfos& getNodeInfo() { return nodeInfo; }

#define VISITOR_FUNCTION(type, name) \
  void visit##type(mathvm::type* node);
  FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION
};
