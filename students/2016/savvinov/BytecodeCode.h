//
// Created by dsavvinov on 11.11.16.
//

#ifndef MATHVM_BYTECODECODE_H
#define MATHVM_BYTECODECODE_H

#include "../../../include/mathvm.h"

namespace mathvm {
class BytecodeCode : public Code {
public:
    BytecodeCode() : Code() {
    }

    Status* execute(vector<Var*>& vars);
};

};
#endif //MATHVM_BYTECODECODE_H
