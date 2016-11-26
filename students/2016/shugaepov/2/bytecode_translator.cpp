#include "../../../../include/mathvm.h"
#include "../../../../vm/parser.h"

#include "ast_to_bytecode.h"

namespace mathvm
{

Status* BytecodeTranslatorImpl::translateBytecode(const string &program, InterpreterCodeImpl **code)
{
    return nullptr;
}

Status* BytecodeTranslatorImpl::translate(const string &program, Code **code)
{
    Parser parser;
    Status* status = parser.parseProgram(program);
    if (status->isError())
        return status;

    ast_to_bytecode_visitor* visitor = new ast_to_bytecode_visitor();

    visitor->translate(*code, parser.top());

    delete visitor;

    return status;
}

} // mathvm


