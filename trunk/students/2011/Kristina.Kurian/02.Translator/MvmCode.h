#ifndef _MVMCODE_H_
#define _MVMCODE_H_

#include "mathvm.h"

struct MvmCode: mathvm::Code {
    mathvm::Status* execute(std::vector<mathvm::Var*>& vars) {
        disassemble();
        return 0;
    }
};

#endif
