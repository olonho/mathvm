#ifndef AST_PRINTER_H__
#define AST_PRINTER_H__

#include <vector>
#include <list>
#include <iostream>

#include "mathvm.h"
#include "parser.h"
#include "ast_source_translator.h"

using namespace mathvm;

class AstPrinter : public Translator {
  public:
    AstPrinter(): mTranslator(new AstSourceTranslator) {}
    ~AstPrinter() {delete mTranslator;}

    virtual Status* translate(const string& program, Code** code)  {
          Parser parser;
          Status* status = parser.parseProgram(program);
          if (status->isError()) return status;
          // Implement printer, using parser.top() as root of the AST
          mTranslator->visitTop(parser.top());
          std::cout << mTranslator->source() << std::endl;
          return Status::Ok();
    }

  private:
    AstSourceTranslator* mTranslator;
};

#endif
