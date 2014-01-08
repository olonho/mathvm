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

        const Bytecode* bytecode() const {
            return &_bytecode;
        }

        virtual void disassemble(ostream& out) const {
            _bytecode.dump(out);
        }


    };

    class BytecodeCode : public Code {
        map<string, uint16_t> globalVars_;
    public:
        virtual Status* execute(vector<Var*>& vars);

        inline map<string, uint16_t>* globalVars() {
            return &globalVars_;
        }

        inline const map<string, uint16_t>* globalVars() const {
            return &globalVars_;
        }

    };
}

#endif

