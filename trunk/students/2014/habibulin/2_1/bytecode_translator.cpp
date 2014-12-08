#include "mathvm.h"
#include "parser.h"
#include "my_utils.h"
#include "typechecking.h"
//#include "bytecode_generator.h"

namespace mathvm {

Status* BytecodeTranslatorImpl::translate(const string& program, Code* *code) {
    DEBUG_MSG("translation started");
    Parser parser;
    Status* status = parser.parseProgram(program);
    if (status->isError()) {
        return status;
    }
    delete status;
    DEBUG_MSG("parsing finished");
    TypeChecker typeChecker;
    typeChecker.check(parser.top());
    DEBUG_MSG("typechecking finished");
    if(typeChecker.status().isError()) {
        string cause = typeChecker.status().errCause();
        size_t pos = typeChecker.status().errPos();
        return Status::Error(cause.c_str(), pos);
    }
//    InterpreterCodeImpl* iCode = new InterpreterCodeImpl();
//    BytecodeGenerator bcGenerator(iCode);
//    DEBUG_MSG("translation starts");
//    try {
//        bcGenerator.visitProgram(parser.top());
//    } catch (TranslatorException& e) {
//        DEBUG_MSG("translation failed");
//        delete iCode;
//        return Status::Error(e.what(), e.source());
//    } catch (ExceptionWithMsg& e) {
//        DEBUG_MSG("translation failed");
//        delete iCode;
//        return Status::Error(e.what(), 0);
//    }

//    *code = iCode;
//    DEBUG_MSG("translation finished");
//    iCode->disassemble();
    return Status::Ok();
}

}
