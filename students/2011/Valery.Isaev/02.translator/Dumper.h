#ifndef _DUMPER_H_
#define _DUMPER_H_

#include "Bytecode.h"

struct Dumper: Bytecode {
    mathvm::Status* execute(std::vector<mathvm::Var*> vars) {
        dump();
        return 0;
    }
};

#endif
