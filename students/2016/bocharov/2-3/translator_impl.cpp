#include "bytecode_generator.h"
#include "interpretable_code.h"

#include "mathvm.h"
#include "parser.h"
#include "visitors.h"

#include <memory>

namespace mathvm {

Status* BytecodeTranslatorImpl::translate(const string& program, Code* *code)
{
    Parser parser;
    std::unique_ptr<Status> status(parser.parseProgram(program));
    if (status && status->isError()) {
        return status.release();
    }

    *code = new InterpretableCode;
    BytecodeGeneratorVisitor codegen(*code);
    try {
        codegen.generateCode(parser.top());
    } catch (const CodeGenerationError& e) {
        return Status::Error(e.what(), e.position());
    }

    return Status::Ok();
}

Translator* Translator::create(const string& impl) {
    if (impl == "" || impl == "intepreter") {
        return new BytecodeTranslatorImpl();
    }
    if (impl == "jit") {
        throw std::runtime_error("jit not implemented yet");
        //return new MachCodeTranslatorImpl();
    }
    assert(false);
    return 0;
}

}
