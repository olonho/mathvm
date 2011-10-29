#pragma once
#include <string>
#include <mathvm.h>
#include <ast.h>
#include "Executable.h"
#include "parse.h"
#include "CodeVisitor.h"
#include "TypeVisitor.h"
#include "SymbolVisitor.h"

class MyTranslator: public mathvm::Translator {
  mathvm::Status status;
  SymbolVisitor *symbolVisitor;
  TypeVisitor* typeVistior;
  CodeVisitor *codeVisitor;
  mathvm::Parser* parser;

  public:
 
  virtual ~MyTranslator() {
    delete symbolVisitor;
    delete typeVistior;
    delete codeVisitor;
    delete parser;
  }  

  virtual mathvm::Status* translate(const std::string& program, mathvm::Code* *code) {
    throw new TranslationException("Translation to mathvm:::Code is not implemented", 0);
  }

  mathvm::Status* translate(const std::string& program, Executable* *executable) {
    //try {
      status = mathvm::Status();
      parser = new mathvm::Parser();
      parseExpr(*parser, program);
      symbolVisitor = new SymbolVisitor(parser->top());
      symbolVisitor->visit();
      symbolVisitor->print(std::cout);
      typeVistior = new TypeVisitor(parser->top());
      typeVistior->visit();
      codeVisitor = new CodeVisitor(parser->top(), 
                                    symbolVisitor->getFunctionContexts(), 
                                    symbolVisitor->getFunctionNodeToIndex(),
                                    symbolVisitor->getIndexToFunctionNode(),
                                    typeVistior->getNodeInfo());

      codeVisitor->visit();
      codeVisitor->getExecutable().disassemble(std::cout);
      *executable = &codeVisitor->getExecutable();
    /*}
    catch (TranslationException* ex) {
      uint32_t position = 0;
      mathvm::AstNode* node = ex->getNode();
      if (node) {
        position = node->position();
      }
      status = mathvm::Status(ex->what(), position);
      throw;
    } */
    return &status;
  }
};
