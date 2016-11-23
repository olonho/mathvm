#pragma once

#include "mathvm.h"

#include <string>

namespace mathvm {

    struct ResultBytecode : Code {
        virtual Status *execute(vector<Var *> &vars) {
            return Status::Ok();
        }

        virtual void disassemble(ostream &out = cout, FunctionFilter *filter = 0) {
            const std::string topFunctionName = "<top>";
            auto topFunction = functionByName(topFunctionName);
            topFunction->disassemble(out);
        }
    };

} // mathvm

