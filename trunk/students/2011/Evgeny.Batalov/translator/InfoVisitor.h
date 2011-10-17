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

typedef std::vector<std::string> Strings;
typedef std::pair<std::string, size_t> SymbolUse; //symbol used, function_id
typedef std::vector<SymbolUse> SymbolsUse;

bool symUsed(const SymbolsUse& a, const std::string& str);
bool symUsed(const Strings& a, const std::string& str);

struct FunctionContext {
  size_t id;
  std::string funcName;
  Strings parameters;
  Strings locals;
  //track use of external symbols only
  SymbolsUse iUsedSymbols;
  SymbolsUse mySymbolsUsed; //I have to put following symbols on stack when I call functions
  //caller have to put following symbols on stack when call me
  SymbolsUse needForClosure;//= (needForClosure of all children + iUsedSymbols) - mySymbolsUsed
  std::vector<size_t> calledFuncs;

  void useMySymbol(std::string sym, size_t userFuncId) {
    if (symUsed(mySymbolsUsed, sym))
      return;
    if (symUsed(parameters, sym) || symUsed(locals, sym)) {
      mySymbolsUsed.push_back(SymbolUse(sym, userFuncId));
    } else {
      throw new TranslationException("use of symbol " + sym  + "  not defined in my scope");
    }
  }
};

void useSymbol(size_t userFuncId, std::string sym, size_t symFuncId, std::vector<FunctionContext>& contexts);

typedef std::vector<FunctionContext> FunctionContexts;
typedef std::map<std::string, std::vector<size_t> > VarFuncContexts;

void genClosures(FunctionContext& cont, FunctionContexts& conts);

class InfoVisitor: public mathvm::AstVisitor {

  mathvm::AstFunction* topAstFunc;
  FunctionContexts funcContexts;
  VarFuncContexts varFuncContexts;
  size_t curFuncId;

  void analizeError(std::string str = "");
  FunctionContext& findFunc(std::string name);

  void pushParameters(mathvm::AstFunction* node, size_t func);
  void popParameters(mathvm::AstFunction* node);
  void pushScope(mathvm::Scope* node);
  void popScope(mathvm::Scope* node);

  public:
  InfoVisitor(mathvm::AstFunction* top);
  void visit();
  void print(std::ostream& out);

#define VISITOR_FUNCTION(type, name) \
  void visit##type(mathvm::type* node);
  FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION
};
