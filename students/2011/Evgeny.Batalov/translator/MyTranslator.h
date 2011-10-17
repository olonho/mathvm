#pragma once
#include <string>
#include <mathvm.h>
#include <ast.h>
#include "MyCode.h"
#include "parse.h"
#include "CodeVisitor.h"
#include "InfoVisitor.h"

class MyTranslator: public mathvm::Translator {
  mathvm::Status status;
  InfoVisitor *infoVisitor;
  CodeVisitor *codeVisitor;

  public:
  virtual mathvm::Status* translate(const std::string& program, mathvm::Code* *code) {
    try {
      status = mathvm::Status();
      mathvm::Parser *p = new mathvm::Parser();
      parseExpr(*p, program);
      infoVisitor = new InfoVisitor(p->top());
      //codeVisitor = new CodeVisitor(p.top());
      infoVisitor->visit();
      infoVisitor->print(std::cout);
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
