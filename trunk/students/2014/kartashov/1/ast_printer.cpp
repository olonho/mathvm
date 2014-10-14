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

    AstPrinter(): _translator(new AstSourceTranslator) {}
    ~AstPrinter() {delete _translator;}

    virtual Status* translate(const string& program, Code* *code)  {
          Parser parser;
          Status* status = parser.parseProgram(program);
          if (status && status->isError()) return status;
          _translator->start(parser.top());
          std::cout << _translator->source() << std::endl;
          return new Status();
    }

  private:
    AstSourceTranslator* _translator;
};

Translator* Translator::create(const string& impl) {
   if (impl == "printer") {
     return new AstPrinter();
   } else {
      return NULL;
   }
}


