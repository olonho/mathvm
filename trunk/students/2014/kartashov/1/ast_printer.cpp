#include <vector>
#include <list>
#include <iostream>

#include "mathvm.h"
#include "parser.h"
#include "ast_source_translator.h"

using namespace mathvm;

class AstPrinter : public Translator {
  public:
    typedef std::list<std::string> SourceText;
    typedef std::list<SourceText> SourceTextStream;

    AstPrinter(): mTranslator(new AstSourceTranslator) {}
    ~AstPrinter() {delete mTranslator;}

    virtual Status* translate(const string& program, Code* *code)  {
          Parser parser;
          Status* status = parser.parseProgram(program);
          if (status && status->isError()) return status;
          mTranslator->visitTop(parser.top());
          std::cout << mTranslator->source() << std::endl;
          return new Status();
    }

  private:
    AstSourceTranslator* mTranslator;
};

Translator* Translator::create(const string& impl) {
   if (impl == "printer") {
     return new AstPrinter();
   } else {
      return NULL;
   }
}


