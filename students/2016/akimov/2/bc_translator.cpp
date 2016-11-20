#include "bc_interpreter.h"
#include "bc_gen.h"
#include "preprocessor.h"

#include <mathvm.h>
#include <parser.h>

namespace mathvm {

Status* BytecodeTranslatorImpl::translate(const string& program, Code* *code) {
    InterpreterCodeImpl* interpreterCode = new InterpreterCodeImpl();
    *code = interpreterCode;
    return translateBytecode(program, &interpreterCode);
}

Status* BytecodeTranslatorImpl::translateBytecode(const string& program,
                                                  InterpreterCodeImpl* *code) {
    Parser parser;
    Status* status = parser.parseProgram(program);
    if (status->isError()) {
        return status;
    }

    try {
        Preprocessor preprocessor;
        parser.top()->node()->visit(&preprocessor);

        BytecodeGenerator generator(*code, &preprocessor);
        generator.processFunction(parser.top(), true);

        (*code)->disassemble(cerr);
    } catch (ErrorInfoHolder* error) {
        return Status::Error(error->getMessage(), error->getPosition());
    }

    return Status::Ok();
}


Translator* Translator::create(const string& impl) {
    if (impl.empty() || impl == "interpreter") {
        return new BytecodeTranslatorImpl();
    }
    return nullptr;
}

}
