#include "mathvm.h"
#include "parser.h"

namespace mathvm {

struct AstPrinter : public Translator {
  virtual Status *translate(const string &program, Code **code) {
    Parser parser;
    Status *status = parser.parseProgram(program);
    if (status->isError()) return status;
//    AstFunction* topfunc = parser.top();
    return new Status("No executable code produced");
  }
};

Translator *Translator::create(const string &impl) {
  if (impl == "printer") {
    return new AstPrinter();
  } else {
    return NULL;
  }
}

}