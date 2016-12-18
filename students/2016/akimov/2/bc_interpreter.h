#pragma once

#include <mathvm.h>
#include "bc_jit.h"

namespace mathvm {

class InterpreterCodeImpl : public Code {
public:
    InterpreterCodeImpl() {}
    ~InterpreterCodeImpl() {}
    virtual Status* execute(vector<Var*>& vars) override;

    BytecodeFunction* functionByName(const string& name) {
        return dynamic_cast<BytecodeFunction*>(Code::functionByName(name));
    }

    BytecodeFunction* functionById(uint16_t id) {
        return dynamic_cast<BytecodeFunction*>(Code::functionById(id));
    }

    BytecodeJITHelper* getJitHelper() {
        return &_jitHelper;
    }
private:
    BytecodeJITHelper _jitHelper;
};

}
