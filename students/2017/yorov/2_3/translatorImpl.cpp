#include "parser.h"
#include "translator.h"
#include "bytecodeVisitor.h"
#include "interpreter.h"

namespace mathvm {
    Translator* Translator::create(const string &impl) {
        return new BytecodeTranslator();
    }

    Status* BytecodeTranslator::translate(const string& program, Code** code) {
      Parser parser{};
      Status* status = parser.parseProgram(program);
      if (status->isError()) {
          return status;
      }
      if (*code == nullptr) {
          *code = new Interpreter(std::cout);
      }
      BytecodeVisitor* visitor = new BytecodeVisitor(*code);
      status = visitor->start(parser.top());
      delete visitor;
      return status;
    }
}
