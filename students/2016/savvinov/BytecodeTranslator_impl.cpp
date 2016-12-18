#include "../../../include/mathvm.h"
#include "../../../vm/parser.h"
#include "BytecodeTranslatorVisitor.h"

namespace mathvm {

Status* BytecodeTranslatorImpl::translate(const string &program, Code **code) {
    Parser parser;

    Status * parseStatus = parser.parseProgram(program);
    if (parseStatus->isError()) {
        return parseStatus;
    }

    BytecodeTranslatorVisitor visitor{};
    FunctionNode * topLevel = parser.top()->node()->asFunctionNode();
    visitor.code->addFunction(new BytecodeFunction(parser.top()));
    topLevel->visit(&visitor);
//    visitor.getCode()->disassemble(std::cout);
    *code = visitor.getCode();
    return Status::Ok();
}

Status *BytecodeTranslatorImpl::translateBytecode(const string &program, InterpreterCodeImpl **code) {
    return nullptr;
}

}   // end of mathvm namespace