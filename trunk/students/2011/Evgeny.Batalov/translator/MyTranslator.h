#pragma once
#include <string>
#include <mathvm.h>
#include <ast.h>
#include "MyCode.h"
#include "parse.h"
#include "CodeVisitor.h"
#include "SymbolVisitor.h"

class MyTranslator: public mathvm::Translator {
  mathvm::Status status;
  SymbolVisitor *symbolVisitor;
  CodeVisitor *codeVisitor;

  public:
  virtual mathvm::Status* translate(const std::string& program, mathvm::Code* *code) {
    try {
      status = mathvm::Status();
      mathvm::Parser *p = new mathvm::Parser();
      parseExpr(*p, program);
      symbolVisitor = new SymbolVisitor(p->top());
      //codeVisitor = new CodeVisitor(p.top());
      symbolVisitor->visit();
      symbolVisitor->print(std::cout);
      //codeVisitor->visit();
      //codeVisitor->getCode().disassemble(std::cout);
      //*code = &codeVisitor->getCode();
    }
    catch (TranslationException* ex) {
      status = mathvm::Status(ex->what());
    }
    return &status;
  }
};
