#include "SymbolVisitor.h"

SymbolVisitor::InfoVisitor(mathvm::AstFunction* top) {
  topAstFunc = top;
}

void SymbolVisitor::visit() { 
  mathvm::Scope* scope = new mathvm::Scope(0);
  scope->declareFunction(topAstFunc->node());
  pushScope(scope);
  topAstFunc->node()->visit(this);
  popScope(scope);
}

void SymbolVisitor::analizeError(std::string str) { 
  throw new TranslationException("Error during code analizing: " + str + "\n");
}

void SymbolVisitor::visitBinaryOpNode(mathvm::BinaryOpNode* node) {    
  node->visitChildren(this);
}

void SymbolVisitor::visitUnaryOpNode(mathvm::UnaryOpNode* node) {
  node->visitChildren(this);
}

void SymbolVisitor::visitStringLiteralNode(mathvm::StringLiteralNode* node) {    
  node->visitChildren(this);
}

void SymbolVisitor::visitDoubleLiteralNode(mathvm::DoubleLiteralNode* node) {
  node->visitChildren(this);
}

void SymbolVisitor::visitIntLiteralNode(mathvm::IntLiteralNode* node) {
  node->visitChildren(this);
}

void SymbolVisitor::visitLoadNode(mathvm::LoadNode* node) {
  node->visitChildren(this);
  useSymbol(curFuncId, node->var()->name(), varFuncContexts.topSymbolData(node->var()->name()), funcContexts);
}

void SymbolVisitor::visitStoreNode(mathvm::StoreNode* node) {
  node->visitChildren(this);
  useSymbol(curFuncId, node->var()->name(), varFuncContexts.topSymbolData(node->var()->name()), funcContexts);
}

void SymbolVisitor::visitForNode(mathvm::ForNode* node) {
  node->visitChildren(this);
  useSymbol(curFuncId, node->var()->name(), varFuncContexts.topSymbolData(node->var()->name()), funcContexts);
}

void SymbolVisitor::visitWhileNode(mathvm::WhileNode* node) {
  node->visitChildren(this);
}

void SymbolVisitor::visitIfNode(mathvm::IfNode* node) {
  node->visitChildren(this);
}

void SymbolVisitor::visitFunctionNode(mathvm::FunctionNode* node) {
  using namespace mathvm;
  
  FunctionContext& func = funcContexts[funcDefs.topSymbolData(node->name())];
  size_t oldFuncId = curFuncId;
  curFuncId = func.id;

  pushParameters(node, curFuncId);
  node->body()->visit(this);
  popParameters(node);
  
  genClosures(funcContexts[curFuncId], funcContexts);
  curFuncId = oldFuncId;
}

void SymbolVisitor::visitBlockNode(mathvm::BlockNode* node) {
  using namespace mathvm;
  pushScope(node->scope());
  node->visitChildren(this);
  popScope(node->scope());
}

void SymbolVisitor::pushParameters(mathvm::FunctionNode* node, size_t funcId) {
  for(size_t i = 0; i < node->parametersNumber(); ++i) {
    varFuncContexts.pushSymbolData(node->parameterName(i), funcId);
  }
}

void SymbolVisitor::popParameters(mathvm::FunctionNode* node) {
  for(size_t i = 0; i < node->parametersNumber(); ++i) {
    varFuncContexts.popSymbolData(node->parameterName(i));
  }
}

void SymbolVisitor::pushScope(mathvm::Scope* node) {

  mathvm::Scope::VarIterator it(node);
  while(it.hasNext()) {
    mathvm::AstVar* var = it.next();
    funcContexts[curFuncId].locals.push_back(var->name());
    varFuncContexts.pushSymbolData(var->name(), curFuncId);
  }
  mathvm::Scope::FunctionIterator fit(node);
  while(fit.hasNext()) {
    mathvm::AstFunction* funcNode = fit.next(); 
    FunctionContext func;
    func.id = funcContexts.size();
    func.funcName = funcNode->name();
    for(size_t i = 0; i < funcNode->parametersNumber(); ++i) {
       func.parameters.push_back(funcNode->parameterName(i));
    }
    funcContexts.push_back(func);
    funcDefs.pushSymbolData(func.funcName, func.id);
    funcNodeToIndex[funcNode->node()] = func.id;
    indexToFuncNode[func.id] = funcNode->node();
  }
  fit = mathvm::Scope::FunctionIterator(node);
  while(fit.hasNext()) {
    fit.next()->node()->visit(this);
  }
}

void SymbolVisitor::popScope(mathvm::Scope* node) {

  mathvm::Scope::VarIterator it(node);
  while(it.hasNext()) {
    mathvm::AstVar* var = it.next();
    varFuncContexts.popSymbolData(var->name());
  }
  mathvm::Scope::FunctionIterator fit(node);
  while(fit.hasNext()) {
    funcDefs.popSymbolData(fit.next()->name());
  }    
}

void SymbolVisitor::visitCallNode(mathvm::CallNode* node) {
  using namespace mathvm;
  node->visitChildren(this);
  FunctionContext& caller = funcContexts[curFuncId];
  FunctionContext& callee =   funcContexts[funcDefs.topSymbolData(node->name())];
  caller.calledFuncs.push_back(callee.id); 
}

void SymbolVisitor::visitReturnNode(mathvm::ReturnNode* node) {
  using namespace mathvm;
  node->visitChildren(this);
}

void SymbolVisitor::visitPrintNode(mathvm::PrintNode* node) {
  using namespace mathvm;
  node->visitChildren(this);
}

void genClosures(FunctionContext& cont, FunctionContexts& conts) {
  std::vector<size_t>::iterator it = cont.calledFuncs.begin();
  //for(; it != cont.calledFuncs.end(); ++it) {
  //  genClosures(conts[*it], conts);
  //}
  
  //(needForClosure of all children + iUsedSymbols) - mySymbolsUsed
  it = cont.calledFuncs.begin();
  SymbolsUse::iterator it1;
  for(; it != cont.calledFuncs.end(); ++it) {
    it1 = conts[*it].needForClosure.begin();
    for(; it1 != conts[*it].needForClosure.end(); ++it1) {
      if (symUsed(cont.needForClosure, it1->first) || symUsed(cont.mySymbolsUsed, it1->first)) {
        continue;
      } else {
        cont.needForClosure.push_back(*it1);
      }
    }
  }

  it1 = cont.iUsedSymbols.begin();
  for(; it1 != cont.iUsedSymbols.end(); ++it1) {
    cont.needForClosure.push_back(*it1);
  }
}

bool symUsed(const SymbolsUse& a, const std::string& str, size_t userId) {
  SymbolsUse::const_iterator it = a.begin();
  for(; it != a.end(); ++it) {
    if (it->first == str && it->second == userId)
      return true;
  }
  return false;
}

bool symUsed(const Strings& a, const std::string& str) {
  Strings::const_iterator it = a.begin();
  for(; it != a.end(); ++it) {
    if (*it == str)
      return true;
  }
  return false;
}

void SymbolVisitor::print(std::ostream& out) {
  FunctionContexts::iterator func = funcContexts.begin();
  for(; func != funcContexts.end(); ++func) {
    out << std::endl << "----------------------------------------" << std::endl;
    out << "Function " << func->funcName << " id: " << func->id << std::endl;
    Strings::iterator str_it = func->parameters.begin();
    out << "Parameters: " << std::endl;
    for(; str_it != func->parameters.end(); ++str_it) {
      out << *str_it << ",";
    }
    out << std::endl << "Locals: " << std::endl;
    str_it = func->locals.begin();
    for(; str_it != func->locals.end(); ++str_it) {
      out << *str_it << ",";
    }
    out << std::endl << "I use external symbols:" << std::endl;
    SymbolsUse::iterator sym_it = func->iUsedSymbols.begin();
    for(; sym_it != func->iUsedSymbols.end(); ++sym_it) {
      out << sym_it->first << " " << sym_it->second  << ",";
    }
    out << std::endl << "My symbols used:" << std::endl;
    sym_it = func->mySymbolsUsed.begin();
    for(; sym_it != func->mySymbolsUsed.end(); ++sym_it) {
      out << sym_it->first << ",";
    }
    out << std::endl << "I need for closure:" << std::endl;
    sym_it = func->needForClosure.begin();
    for(; sym_it != func->needForClosure.end(); ++sym_it) {
      out << sym_it->first << " " << sym_it->second  << ",";
    }

    out << std::endl << "I call functions:" << std::endl;
    std::vector<size_t>::iterator int_it = func->calledFuncs.begin();
    for(; int_it != func->calledFuncs.end(); ++int_it) {
      out << *int_it << ",";
    }
    out << std::endl << "--------------------------------------" << std::endl;
  }
}

void useSymbol(size_t userFuncId, std::string sym, size_t symFuncId, std::vector<FunctionContext>& contexts) {
  FunctionContext& user = contexts[userFuncId];
  if (user.id == symFuncId)
    return; //for closure we are not interrested in use of local symbols 
  if (symUsed(user.iUsedSymbols, sym))
    return; //symbol marked in use yet
  user.iUsedSymbols.push_back(SymbolUse(sym, symFuncId));
  contexts[symFuncId].useMySymbol(sym, user.id);
  if (symUsed(user.locals, sym))
    throw new TranslationException("binding local symbol " + sym +  " as closured");
  if (symUsed(user.parameters, sym))
    throw new TranslationException("binding local parameter " + sym  + " as closured");
}
