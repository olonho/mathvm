#pragma once

#include <string>
#include <stdexcept>

#include "mathvm.h"

#ifdef DEBUG
#define DBG(x) cout << #x << " = " << (x) << endl;
#else
#define DBG(x)  
#endif

inline void my_error(std::string cause) {
    throw std::runtime_error(cause);
}


namespace mathvm {

class BytecodeEmitterVisitor;

struct InterpreterCodeImpl : Code {
    InterpreterCodeImpl(BytecodeEmitterVisitor* visitor_): visitor(visitor_) {
    }

    ~InterpreterCodeImpl() = default;
    Status* execute(vector<Var*>& vars) override;

    BytecodeEmitterVisitor* visitor;
};


}
