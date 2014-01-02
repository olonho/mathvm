#ifndef BYTECODE_H
#define	BYTECODE_H

#include "mathvm.h"
#include "bytecode.h"

namespace mathvm {

    class BytecodeFunction : public TranslatedFunction {
        Bytecode _bytecode;

    public:

        BytecodeFunction(AstFunction* function) :
        TranslatedFunction(function) {
        }

        Bytecode* bytecode() {
            return &_bytecode;
        }

        virtual void disassemble(ostream& out) const {
            _bytecode.dump(out);
        }
    };

    class BytecodeCode : public Code {
    public:
        virtual Status* execute(vector<Var*>& vars) {
            return NULL;
        }
    };
}

#endif

