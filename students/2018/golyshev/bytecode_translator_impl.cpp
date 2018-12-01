#include <mathvm.h>
#include <parser.h>
#include "bytecode_translator_visitor.hpp"
#include "interpreter_code_impl.hpp"

using namespace mathvm;
using std::string;

Status* BytecodeTranslatorImpl::translateBytecode(string const& program, InterpreterCodeImpl** code) {
    Parser parser;

    Status* parseStatus = parser.parseProgram(program);

    if (parseStatus->isOk()) {
        *code = new InterpreterCodeImpl();
        BytecodeTranslatorVisitor visitor(*code);

        visitor.translateProgram(parser.top());
    }

    return parseStatus;
}

Status* BytecodeTranslatorImpl::translate(string const& program, Code** code) {
    InterpreterCodeImpl* codeImpl = nullptr;

    Status* status = translateBytecode(program, &codeImpl);
    *code = codeImpl;

    return status;
}