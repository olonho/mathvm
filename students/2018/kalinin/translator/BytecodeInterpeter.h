//
// Created by Владислав Калинин on 17/11/2018.
//

#ifndef MATHVM_BYTECODEINTERPETER_H
#define MATHVM_BYTECODEINTERPETER_H

#include "BytecodeGenerator.h"
#include "Context.h"

namespace mathvm {
    class Context;

    class StackContext;

    union Val {
        Val() {}

        Val(uint64_t i) : i(i) {}

        Val(double d) : d(d) {}

        Val(uint16_t i16) : i16(i16) {}

        double d;
        uint64_t i;
        uint16_t i16;
    };

    class BytecodeInterpeter : public Code {
        Context *globalCtx{};
        vector<Val> stack{};
        StackContext *ctx{};
        uint32_t offset = 0;

    public:
        explicit BytecodeInterpeter(Context *globalCtx) : globalCtx(globalCtx) {
            init();
        };

        void interpate(Instruction ins, uint32_t offset);

        Status *execute(vector<Var *> &vars);

    private:
        void init();

    };
}//mathvm

#endif //MATHVM_BYTECODEINTERPETER_H
