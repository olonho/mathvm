#include "machcode_translator.hpp"
#include "jit.hpp"

namespace mathvm {

MachCodeImpl::MachCodeImpl(InterpreterCodeImpl& code): env(code) {
}

Status *MachCodeImpl::execute(vector<Var*>& vars) {
    env.execute();
    return Status::Ok();
}

MachCodeTranslatorImpl::MachCodeTranslatorImpl() {
}

MachCodeTranslatorImpl::~MachCodeTranslatorImpl() {
}

Status* MachCodeTranslatorImpl::translateMachCode(
    const string& program, MachCodeImpl* *code)
{
    BytecodeTranslatorImpl bytecode_translator;
    Code *bytecode;
    Status *status = bytecode_translator.translate(program, &bytecode);
    *code = new MachCodeImpl(
        *dynamic_cast<InterpreterCodeImpl*>(bytecode));
    return status;
}

Status *MachCodeTranslatorImpl::translate(const string& program, Code* *code) {
    MachCodeImpl *mcode;
    Status *status = translateMachCode(program, &mcode);
    *code = mcode;
    return status;
}

}
