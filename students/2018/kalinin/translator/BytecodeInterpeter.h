//
// Created by Владислав Калинин on 17/11/2018.
//

#ifndef MATHVM_BYTECODEINTERPETER_H
#define MATHVM_BYTECODEINTERPETER_H

#include "BytecodeGenerator.h"

namespace mathvm {
    class Context;

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
        Context *ctx{};
        vector<Val> stack{};

    public:
        explicit BytecodeInterpeter(Context *ctx) : ctx(ctx) {};

        void interpate(Instruction ins, uint32_t offset);

        Status *execute(vector<Var *> &vars);

    };
}//mathvm

#endif //MATHVM_BYTECODEINTERPETER_H
