#include <parser.h>
#include "mathvm.h"
#include "formatter.h"
#include "bytecode_writer.h"

namespace mathvm {

class PrintTranslator : public Translator {
public:
    Status* translate(const string& program, Code* *) override;
};

class SourceToBytecodeTranslator : public Translator {
public:
    Status* translate(const string& program, Code* *) override;
};


Translator* Translator::create(const string& impl) {
    if (impl == "" || impl == "interpreter") {
        //return new BytecodeTranslatorImpl();
        return 0;
    }
    if (impl == "printer") {
        return new PrintTranslator();
    }
    if (impl == "src2bc") {
        return new SourceToBytecodeTranslator();
    }
    if (impl == "jit") {
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

class BytecodeImpl : public Code {
public:
    virtual Status* execute(vector<Var*>& vars) override {

        return Status::Ok();
    }
};

mathvm::Status *mathvm::SourceToBytecodeTranslator::translate(
        const std::string &program, mathvm::Code **pCode) {
    Parser parser;
    Status *s = parser.parseProgram(program);

    if (s->isError()) {
        return s;
    }

    *pCode = new BytecodeImpl;
    BytecodeWriter writer(*pCode, parser.top());
    try {
        parser.top()->node()->visit(&writer);
    } catch (BytecodeWriter::PositionalException &e) {
        return Status::Error(e.what(), e.position);
    }

    (**pCode).disassemble();
    return Status::Ok();
}

Status* BytecodeTranslatorImpl::translate(const string &program, Code **code) {
    Parser parser;
    Status *s = parser.parseProgram(program);

    if (s->isError()) {
        return s;
    }

    delete s;
    //TODO
    return Status::Ok();
}

Status* BytecodeTranslatorImpl::translateBytecode(const string &program,
                                                  InterpreterCodeImpl **code) {
    return Status::Ok();
}

}
