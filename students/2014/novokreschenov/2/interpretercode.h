#ifndef INTERPRETERCODE_H
#define INTERPRETERCODE_H

#include "mathvm.h"
#include "contextmanager.h"
#include "exceptions.h"

namespace mathvm {

class InterpreterCodeImpl : public Code
{
    InterpreterStack _stack;
    ContextManager _contextManager;
    std::vector<Bytecode*> _bytecodes;
    std::vector<uint32_t> _indexes;

    void executeBytecode();
    void executeBytecodeInsn(Instruction insn);
    Bytecode* bytecode();
    void addNewBytecode(Bytecode* bytecode);
    void removeLastBytecode();

    uint32_t bytecodeIndex();
    void shiftBytecodeIndex(int16_t shift);
    void setBytecodeIndex(uint32_t index);

    static int64_t compare(double upper, double lower);
    static int64_t compare(int64_t upper, int64_t lower);

    void readCtxIdVarId(uint16_t& ctxId, uint16_t& varId);
    void readVarId(uint16_t& varId);

#define EXECUTE_INSN(b, d, l)         \
    void execute##b();

    FOR_BYTECODES(EXECUTE_INSN)
#undef EXECUTE_INSN

    void executeBytecodeFunction(BytecodeFunction* bcFunction);
    void executeNativeFunction(const string* name, const Signature* signature, const void* code);

public:
    virtual ~InterpreterCodeImpl();

    virtual Status* execute(vector<Var *> &vars);
    virtual void disassemble(ostream& out = cout, FunctionFilter* filter = 0);
};

}

#endif // INTERPRETERCODE_H
