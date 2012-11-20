#include <iostream>
#include <vector>
#include <string>

#include "mathvm.h"
#include "ast.h"
#include "parser.h"

#include "BytecodeEmittingAstVisitor.h"

namespace mathvm {

class InterpreterCodeImpl : public Code {
  public:
    virtual Status* execute(std::vector<mathvm::Var*>&) {
      return new Status("InterpreterCodeImpl: Unimplemented");
    }
};

Status* BytecodeTranslatorImpl::translateBytecode(const std::string& program, InterpreterCodeImpl* *code) {
    return new Status("BytecodeTranslatorImpl: Unimplemented");
}

Status* BytecodeTranslatorImpl::translate(const std::string& text, Code* *code) {
    if (*code == 0) {
        *code = new InterpreterCodeImpl();
    }
    Status* status = 0;

    Parser parser;
    status = parser.parseProgram(text);
    if (status && status->isError()) {
        uint32_t line = 0;
        uint32_t offset = 0;
        positionToLineOffset(text,
                             status->getPosition(),
                             line, offset);
        std::cerr << "Parser error: "
                  << status->getError()
                  << " at (" << line << ":" << offset << ")"
                  << std::endl;
        return status;
    }

    mathvm_ext::BytecodeEmittingAstVisitor visitor(*code);
    visitor(parser.top());

    return status;
}

}   // namespace mathvm
