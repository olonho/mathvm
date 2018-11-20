//
// Created by Владислав Калинин on 17/11/2018.
//

#ifndef MATHVM_BYTECODEINTERPETER_H
#define MATHVM_BYTECODEINTERPETER_H

#include "../../../include/mathvm.h"

namespace mathvm {
    class BytecodeInterpeter : public Code {

    public:
        Status *execute(vector<Var *> &vars);

    };
}//mathvm

#endif //MATHVM_BYTECODEINTERPETER_H
