#ifndef MATHVM_INTERPRETER_H
#define MATHVM_INTERPRETER_H

#include "mathvm.h"
#include <vector>
#include "interpreterUtil.h"

namespace mathvm {
    struct Interpreter : Code {
        Interpreter(std::ostream& os);
        virtual Status* execute(std::vector<Var*>& vars) override;

    private:
        static double epsilon;
        void executeFunction(BytecodeFunction* function);
        std::ostream& _os;
        utils::Stack _stack;
        utils::ContextsVariable _variables;
    };
}

#endif // MATHVM_INTERPRETER_H
