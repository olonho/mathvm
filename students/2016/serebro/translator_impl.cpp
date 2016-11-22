#include <parser.h>
#include "formatter.h"
#include "bytecode_writer.h"
#include "interpreter_impl.h"
#include "bytecode_optimizer.h"
#include "translator_impl.h"

namespace mathvm {

class PrintTranslator : public Translator {
public:
    Status* translate(const string& program, Code* *) override;
};

Translator* Translator::create(const string& impl) {
    if (impl == TR_FORMATTER) {
        return new PrintTranslator();
    }
    if (impl == TR_BYTECODE) {
        return new BytecodeTranslatorImpl();
    }
    if (impl == TR_JITCODE) {
        //return new MachCodeTranslatorImpl();
        return 0;
    }
    assert(false);
    return 0;
}


Status *PrintTranslator::translate(const string &program, Code **) {
    string result;
    Formatter f;

    Status *status = f.formatCode(result, program);

    if (status->isOk()) {
        cout << result << endl;
    }

    return status;
}

Status *BytecodeTranslatorImpl::translate(
        const std::string &program, mathvm::Code **pCode) {
    Parser parser;
    Status *s = parser.parseProgram(program);

    if (s->isError()) {
        return s;
    }

    InterpreterCodeImpl *code = new InterpreterCodeImpl;
    BytecodeWriter writer(code, parser.top(), code->getMeta());
    try {
        parser.top()->node()->visit(&writer);
    } catch (BytecodeWriter::PositionalException &e) {
        return Status::Error(e.what(), e.position);
    }

    *pCode = code;
    BytecodeOptimizer optimizer(code, code->getMeta());
    optimizer.removeEmptyBlocks();

    return Status::Ok();
}


}
