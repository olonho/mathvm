#include "mathvm.h"
#include "parser.h"
#include "include/bytecodeVisitor.h"

namespace mathvm {
    Status* BytecodeTranslatorImpl::translate(const std::string &program, Code **code) {
        Parser parser;
        Status *status = parser.parseProgram(program);

        if (status->isError())
            return status;
        delete status;

        BytecodeVisitor visitor;
        status = visitor.process(parser.top());
        *code = visitor.getCode();

        return status;
    }
}
