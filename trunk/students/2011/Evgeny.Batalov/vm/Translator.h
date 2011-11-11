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
  MyTranslator() 
    : symbolVisitor(0)
    , typeVistior(0)
    , codeVisitor(0)
    , parser(0)
  {}
  virtual ~MyTranslator() {
    if (symbolVisitor)
      delete symbolVisitor;
    if (typeVistior)
      delete typeVistior;
    if (codeVisitor)
      delete codeVisitor;
    if (parser)
      delete parser;
  }  

  virtual mathvm::Status* translate(const std::string& program, mathvm::Code* *code) {
    throw new TranslationException("Translation to mathvm:::Code is not implemented", 0);
  }

  mathvm::Status* translate(const std::string& program, Executable* *executable) {
    try {
      status = mathvm::Status();
      parser = new mathvm::Parser();
      parseExpr(*parser, program);
      symbolVisitor = new SymbolVisitor(parser->top());
      symbolVisitor->visit();
      #ifdef VERBOSE
      symbolVisitor->print(std::cout);
      #endif
      typeVistior = new TypeVisitor(parser->top(),
                                    symbolVisitor->getFunctionContexts(),
                                    symbolVisitor->getFunctionNodeToIndex(),
                                    symbolVisitor->getIndexToFunctionNode());
      typeVistior->visit();
      codeVisitor = new CodeVisitor(parser->top(), 
                                    symbolVisitor->getFunctionContexts(),
                                    symbolVisitor->getFunctionNodeToIndex(),
                                    symbolVisitor->getIndexToFunctionNode(),
                                    typeVistior->getNodeInfo());

      codeVisitor->visit();
      #ifdef VERBOSE
      codeVisitor->getExecutable()->disassemble(std::cout);
      #endif
      *executable = codeVisitor->getExecutable();
    }
    catch (TranslationException* ex) {
      uint32_t position = 0;
      mathvm::AstNode* node = ex->getNode();
      if (node) {
        position = node->position();
      }
      status = mathvm::Status(ex->what(), position);
      //throw;
    }
    return &status;
  }
};
