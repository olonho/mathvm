#include "parser.h"
#include <string.h>
#include "visitors.h"
#include "ast_printer.h"
#include "bytecode_translator.h"
#include "mathvm.h"

namespace mathvm {

class ASTPrintTranslator : public Translator {
  Status* translate(const string& program, Code* *code) override {
      Parser parser;
      Status* status = parser.parseProgram(program);
      if (status->isError()) {
          return status;
      }
      std::stringstream sstream;
      ASTPrinter printer(sstream);
      parser.top()->node()->body()->visit(&printer);
      std::cout << sstream.rdbuf();
      return Status::Ok();
  }
};

Translator* Translator::create(const string& impl) {
    if (impl == "printer") {
        return new ASTPrintTranslator();
    }
    if (impl == "" || impl == "translate") {
        return new BytecodeTranslatorImpl();
    }
    if (impl == "jit") {
        //return new MachCodeTranslatorImpl();
    }
    assert(false);
    return 0;
}

}
