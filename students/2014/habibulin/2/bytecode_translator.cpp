#include "mathvm.h"
#include "parser.h"
#include "bytecode_generator.h"

namespace mathvm {

Status* BytecodeTranslatorImpl::translate(const string& program, Code* *code) {
    DEBUG_MSG("translation started");
    Parser parser;
    Status* status = parser.parseProgram(program);
    DEBUG_MSG("parsing finished - checking");
    if (status->isError()) {
        return status;
    }
    DEBUG_MSG("parsing finished - status ok");
    delete status;
    DEBUG_MSG("parsing finished - status removed");
    InterpreterCodeImpl* iCode = new InterpreterCodeImpl();
    BytecodeGenerator bcGenerator(iCode);
    DEBUG_MSG("translation starts");
    try {
        bcGenerator.visitProgram(parser.top());
    } catch (TranslatorException& e) {
        DEBUG_MSG("translation failed");
        delete iCode;
        return Status::Error(e.what(), e.source());
    } catch (ExceptionWithMsg& e) {
        DEBUG_MSG("translation failed");
        delete iCode;
        return Status::Error(e.what(), 0);
    }

    *code = iCode;
    DEBUG_MSG("translation finished");
//    iCode->disassemble();
    return Status::Ok();
}

}
