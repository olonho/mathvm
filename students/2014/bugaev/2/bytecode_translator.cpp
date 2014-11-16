#include "interpreter_code.hpp"
#include "bytecode_generator.hpp"
#include "parser.h"


namespace mathvm
{

Translator *Translator::create(string const &impl)
{
    if (impl == "" || impl == "intepreter")
        return new BytecodeTranslatorImpl();
    return 0;
}


Status *BytecodeTranslatorImpl::translate(
        string const &program,
        Code* *code)
{
    InterpreterCodeImpl *icode = 0;
    Status *status = translateBytecode(program, &icode);
    assert(status);

    if (!status->isError())
        *code = icode;
    return status;
}


Status *BytecodeTranslatorImpl::translateBytecode(
        string const &program,
        InterpreterCodeImpl* *code)
{
    Parser parser;
    Status *ps = parser.parseProgram(program);

    assert(ps);
    if (ps->isError())
        return ps;

    *code = new InterpreterCodeImpl();
    Status *gs = BytecodeGenerator().generate(*code, parser.top());

    assert(gs);
    if (gs->isError()) {
        delete ps;
        return gs;
    }

    delete gs;
    return ps;
}

}
