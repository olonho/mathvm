#include "mathvm.h"
#include "parser.h"
#include "bytecode_generator.h"

namespace mathvm {

Status* BytecodeTranslatorImpl::translate(const string& program, Code* *code) {
    Parser parser;
    Status* status = parser.parseProgram(program);
    if (status->isError()) {
        return status;
    }
    InterpreterCodeImpl* iCode = new InterpreterCodeImpl();
    BytecodeGenerator bcGenerator(iCode);
    DEBUG_MSG("translation starts");
    try {
        bcGenerator.visitProgram(parser.top());
    } catch (TranslatorException e) {
        delete iCode;
        return Status::Error(e.what(), e.source());
    }
    // no error state check !!!!
    *code = iCode;
    //debug
    DEBUG_MSG("translation finished");
    iCode->disassemble();
    return Status::Ok();
}

}
