#include "../../../../include/mathvm.h"
#include "../../../../include/visitors.h"

#ifndef VM_INTERPRETERRF_H
#define VM_INTERPRETERRF_H

namespace mathvm {
    class EmptyInterpreter : public Code {

    public:
        virtual Status *execute(vector<Var *> &vars) override;
    };
}


#endif //VM_INTERPRETERRF_H
