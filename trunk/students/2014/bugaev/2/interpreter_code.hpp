#ifndef INTERPRETER_CODE_HPP
#define INTERPRETER_CODE_HPP

#include "mathvm.h"
#include "ast.h"
#include "asmjit/asmjit.h"


namespace mathvm
{

class InterpreterCodeImpl: public Code
{
public:
    Status *execute(vector<Var *> &);
    uint16_t buildNativeFunction(NativeCallNode *node);

private:
    asmjit::JitRuntime m_runtime;
};

}


#endif // INTERPRETER_CODE_HPP
