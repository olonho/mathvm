#include "mathvm.h"
#include "interpreter_code.h"


namespace mathvm {

Status* BytecodeTranslatorImpl::translate(const string& program,
        Code* *code) {
    // TODO: return InterpreterCodeImpl
    return nullptr;
}


Status* BytecodeTranslatorImpl::translateBytecode(const string& program,
        InterpreterCodeImpl* *code)
{
    return nullptr;
}


void InterpreterCodeImpl::dissasemble(ostream& out=std::cout,
        FunctionFilter* filter=0) {
}

Status* InterpreterCodeImpl::execute(vector<Var*>& vars) {
    return nullptr;
}

}
