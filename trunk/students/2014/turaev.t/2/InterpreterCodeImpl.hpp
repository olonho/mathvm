#ifndef __INTERPRETERCODEIMPL_HPP_
#define __INTERPRETERCODEIMPL_HPP_

#include "mathvm.h"

namespace mathvm {

    class InterpreterCodeImpl : public Code {
    public:
        InterpreterCodeImpl() : bytecode(new Bytecode) {

        }

        void setBytecode(Bytecode *bc) {
            if (bytecode)
                delete bytecode;
            bytecode = bc;
        }

        Bytecode *getBytecode() const {
            return bytecode;
        }

        virtual Status *execute(vector<Var *> &vars) override {
            disassemble(cout);
            return Status::Ok();
        }

        virtual void disassemble(ostream &out, FunctionFilter *filter = 0) override {
            Code::disassemble(out, filter);
        }

    protected:
        Bytecode *bytecode;
    };
}

#endif