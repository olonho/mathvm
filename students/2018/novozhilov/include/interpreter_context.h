#ifndef VIRTUAL_MACHINES_INTERPRETER_CONTEXT_H
#define VIRTUAL_MACHINES_INTERPRETER_CONTEXT_H

#include <mathvm.h>
#include "context.h"

namespace mathvm {
    class InterpreterContext {
    private:
        vector<StackValue> _variables;

    public:
        explicit InterpreterContext(BytecodeFunction *function);

        StackValue getVarById(uint16_t index);

        void setVarById(StackValue var, uint16_t index);
    };
}
#endif //VIRTUAL_MACHINES_INTERPRETER_CONTEXT_H
