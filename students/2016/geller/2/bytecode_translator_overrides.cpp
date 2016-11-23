//
// Created by wimag on 23.11.16.
//

#include <mathvm.h>
#include "bytecode_translator_overrides.h"

using namespace mathvm;


Status *InterpreterCodeImpl::execute(vector < Var * > &vars) {
    return nullptr;
}

Status* BytecodeTranslatorImpl::translate(const string &program, Code **code) {
    Parser parser;
    Status* status = parser.parseProgram(program);
    if(status->isError()){
        return status;
    }
    *code = new InterpreterCodeImpl();
    ast_bytecode_generator(parser.top(), *code).execute();
    return Status::Ok();

}