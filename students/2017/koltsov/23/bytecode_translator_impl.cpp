#include "mathvm.h"
#include "ast.h"
#include "parser.h"
#include "visitors.h"

#include "emitter.h"
#include "interpreter_code.h"
#include "my_interpreter.h"


namespace mathvm {

using std::string;

Status* BytecodeTranslatorImpl::translate(const string& program,
        Code* *code) {
    auto visitor = new BytecodeEmitterVisitor();
    *code = new InterpreterCodeImpl(visitor);
    visitor->setCode(*code);
    return translateBytecode(program, reinterpret_cast<InterpreterCodeImpl**>(code));
}


Status* BytecodeTranslatorImpl::translateBytecode(const string& program,
        InterpreterCodeImpl* *code)
{
    Parser parser;
    Status* status = parser.parseProgram(program);
    if (not status->isError()) {
        (*code)->visitor->emitFromTop(parser.top());
    }

    return status;
}

Status* InterpreterCodeImpl::execute(vector<Var*>& vars) {
    try {
        Interpreter interpreter(this);
        interpreter.assignVars(vars);
        interpreter.mainLoop();
        interpreter.saveVars(vars);
    } catch (runtime_error &e) {
        return Status::Error(e.what());
    }

    return Status::Ok();
}

} // namespace mathvm
