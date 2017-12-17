#pragma once

#include <visitors.h>
#include "context.h"

namespace mathvm {

    class InterpreterCodeImpl : public Code {
    public:

        InterpreterCodeImpl();

        ~InterpreterCodeImpl();

        Status* execute(vector<Var*>&) override;

    private:

        bool executeInstruction(Instruction insn);

        bool hasInstructions();

        void push(Val v);

        void dpush(double d);

        void ipush(int64_t i);

        void spush(id_t s);

        Val pop();

        Val top();

        Context* getContext();

        Context* getContext(id_t ctxId);

        void prepareContext(BytecodeFunction* func);

        void clearContext();

        Bytecode* getBytecode();

    private:
        Context* _currentContext;
        std::vector<Val> stack;
    };
}