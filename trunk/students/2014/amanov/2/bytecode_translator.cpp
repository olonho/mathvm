#include "bytecode_translator.h"
#include "interpreter_code_impl.h"
#include "parser.h"
#include "bytecode_generator.h"


namespace mathvm {


Translator* Translator::create(const string& impl) {
    if (impl == "" || impl == "intepreter") {
        return new BytecodeTranslator();
    }
    assert(false);
    return 0;
}

Status *BytecodeTranslator::translate(const string &program, Code **code)
{
    InterpreterCodeImpl *codeImpl = 0;
    Status *status = translateBytecode(program, &codeImpl);
    *code = codeImpl;
    return status;
}

Status *BytecodeTranslator::translateBytecode(const string &program, InterpreterCodeImpl **code)
{
    Parser parser;
    Status* status = parser.parseProgram(program);
    if (status) {
        if (status->isError())
            return status;
        else
            delete status;
    }
    InterpreterCodeImpl *codeImpl = new InterpreterCodeImpl();
    *code = codeImpl;
    BytecodeGenerator bytecodeGenerator(codeImpl);
    return bytecodeGenerator.generate(parser.top());
}

}
