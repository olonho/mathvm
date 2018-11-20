//
// Created by Владислав Калинин on 17/11/2018.
//

#ifndef MATHVM_BYTECODEINTERPETER_H
#define MATHVM_BYTECODEINTERPETER_H

#include "bytecode_translator_visitor.h"

namespace mathvm {
    class Context;

    class BytecodeInterpeter : public Code {
        Context *ctx{};

    public:
        explicit BytecodeInterpeter(Context *ctx) : ctx(ctx) {};

        Status *execute(vector<Var *> &vars);

    };
}//mathvm

#endif //MATHVM_BYTECODEINTERPETER_H
