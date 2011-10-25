#pragma once
#include <string>
#include <vector>
#include <map>
#include <utility>
#include <mathvm.h>
#include <ast.h>
#include <visitors.h>
#include "MyCode.h"
#include "TranslationException.h"
#include "TranslationUtils.h"

class SymbolVisitor: public mathvm::AstVisitor {

  mathvm::AstFunction* topAstFunc;
  FunctionContexts funcContexts;
  SymbolStack<size_t> varFuncContexts;
  SymbolStack<size_t> funcDefs;

  FunctionNodeToIndex funcNodeToIndex;
  IndexToFunctionNode indexToFuncNode;

  size_t curFuncId;
  void analizeError(std::string str = "");
  
  void pushParameters(mathvm::FunctionNode* node, size_t func);
  void popParameters(mathvm::FunctionNode* node);
  void pushScope(mathvm::Scope* node);
  void popScope(mathvm::Scope* node);

public:
  SymbolVisitor(mathvm::AstFunction* top);
  void visit();
  void print(std::ostream& out);
  FunctionContexts& getFunctionContexts() { return funcContexts; } 
  FunctionNodeToIndex& getFunctionNodeToIndex() { return funcNodeToIndex; }
  IndexToFunctionNode& getIndexToFunctionNode() { return indexToFuncNode; }

#define VISITOR_FUNCTION(type, name) \
  void visit##type(mathvm::type* node);
  FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION
};
