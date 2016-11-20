#pragma once

#include "helpers.h"
#include <mathvm.h>
#include <vector>

namespace mathvm {
    using namespace mathvm::details;
    using std :: vector;

    struct ExecutableCode : public Code {
        Status* execute(vector<Var*>& vars);
    private:
        Bytecode* getCurrentBytecode();
    private:
        Context* _env;
    };
} // end namespace mathvm
