#include "parser.h"
#include "translator.h"
#include "bytecodeVisitor.h"

namespace mathvm {
    Translator* Translator::create(const string &impl) {
        if (impl == "translator") {
            return new BytecodeTranslator();
        }
        return nullptr;
    }

    Status* BytecodeTranslator::translate(const string& program, Code** code) {
      Parser parser{};
      Status* status = parser.parseProgram(program);
      if (status->isError()) {
          return status;
      }
      BytecodeVisitor* visitor = new BytecodeVisitor(*code);
      status = visitor->start(parser.top());
      delete visitor;
      return status;
    }
}
