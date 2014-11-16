#include <iostream>
#include "parser.h"
#include "BytecodeVisitor.hpp"
#include "SimpleInterpreter.hpp"

using namespace std;

namespace mathvm {
    Status *BytecodeTranslatorImpl::translateBytecode(string const &source, InterpreterCodeImpl **code) {
        Parser parser;
        Status *status = parser.parseProgram(source);
        if (status && status->isError()) {
            return status;
        }

//        InterpreterCodeImpl *interpreterCode = new InterpreterCodeImpl();
        InterpreterCodeImpl *interpreterCode = new SimpleInterpreter();
        (*code) = interpreterCode;
        Context topContext(interpreterCode);
        topContext.introduceFunction(new BytecodeFunction(parser.top()));

        try {
            BytecodeVisitor visitor(&topContext);
            visitor.visitFunctionNode(parser.top()->node());
            interpreterCode->getBytecode()->addInsn(BC_STOP);
        } catch (TranslationError e) {
            return Status::Error(e.what(), e.where());
        }

        return Status::Ok();
    }

    Status *BytecodeTranslatorImpl::translate(string const &source, Code **code) {
        return translateBytecode(source, (InterpreterCodeImpl **) code);
    }
}