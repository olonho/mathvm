#ifndef bytecode_interpreter_hpp_INCLUDED
#define bytecode_interpreter_hpp_INCLUDED

#include <mathvm.h>

namespace mathvm {

class MathvmCode : public Code {
public:
    virtual Status* execute(vector<Var*>& vars);
};

}

#endif // bytecode_interpreter_hpp_INCLUDED

