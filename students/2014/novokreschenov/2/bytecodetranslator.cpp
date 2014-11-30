#include "mathvm.h"
#include "parser.h"
#include "visitors.h"
#include "bytecodegenerator.h"
#include "interpretercode.h"

namespace mathvm {

Translator* Translator::create(const string& impl) {
    if (impl == "" || impl == "intepreter") {
        return new BytecodeTranslatorImpl();
    }
    if (impl == "jit") {
        //return new MachCodeTranslatorImpl();
    }
    assert(false);
    return 0;
}

Status* BytecodeTranslatorImpl::translate(const string& program, Code* *code) {
    InterpreterCodeImpl* interpreterCode;
    Status* status = translateBytecode(program, &interpreterCode);
    *code = interpreterCode;

    return status;
}

Status* BytecodeTranslatorImpl::translateBytecode(const string &program, InterpreterCodeImpl **code)
{
    Parser parser;
    Status* parserStatus = parser.parseProgram(program);
    if (parserStatus != NULL && parserStatus->isError()) {
        return parserStatus;
    }
    delete parserStatus;

    AstFunction* top = parser.top();
    BytecodeGenerator bytecodeGenerator;
    Status* translatorStatus = bytecodeGenerator.makeBytecode(top, code);

    return translatorStatus;
}

}
