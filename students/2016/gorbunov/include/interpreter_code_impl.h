#ifndef PROJECT_INTERPRETER_CODE_IMPL_H
#define PROJECT_INTERPRETER_CODE_IMPL_H

#include <mathvm.h>
#include <ast.h>
#include "bytecode_metadata..h"
#include <memory>

namespace mathvm
{

    class InterpreterCodeImpl: public Code
    {
    private:
        std::shared_ptr<BytecodeMetadata> bc_meta;
    public:
        InterpreterCodeImpl() {}
        Status *execute(vector<Var*> &);
        uint16_t registerNativeFunction(BytecodeFunction* bf);
        void setBcMeta(std::shared_ptr<BytecodeMetadata> bc_meta) {
            this->bc_meta = bc_meta;
        }

        const BytecodeMetadata* getBcMeta() {
            return bc_meta.get();
        }
    };

}

#endif //PROJECT_INTERPRETER_CODE_IMPL_H
