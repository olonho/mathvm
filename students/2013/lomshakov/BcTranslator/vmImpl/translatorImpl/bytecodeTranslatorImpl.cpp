#include "mathvm.h"
#include "parser.h"
#include "interpreter.h"
#include "bcEmitter.h"

namespace mathvm {


  Status* BytecodeTranslatorImpl::translateBytecode(const string& program, InterpreterCodeImpl* *code) {
    Parser parser = Parser();
    Status* parserStatus = parser.parseProgram(program);

    if (!(parserStatus == 0 || parserStatus->isOk()))
      return parserStatus;

    BytecodeEmitter::getInstance().emitCode(parser.top(), *code);

    return new Status();
  }

  Status *BytecodeTranslatorImpl::translate(const string &program, Code **code) {
    InterpreterCodeImpl* interpreterCode = new InterpreterCodeImpl();
    Status* statusTranslate = 0;

    try {
      statusTranslate = translateBytecode(program, &interpreterCode);
    } catch(std::exception &e) {
      statusTranslate = new Status(e.what(), Status::INVALID_POSITION);
    }

    if (statusTranslate->isOk()) {
      *code = interpreterCode;
    }

    return statusTranslate;
  }
}