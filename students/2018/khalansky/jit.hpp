#ifndef jit_hpp_INCLUDED
#define jit_hpp_INCLUDED

#include <mathvm.h>
#include "bytecode_interpreter.hpp"

namespace mathvm {

class JitEnvImpl;

class JitEnvironment {

    JitEnvImpl *inner;

public:

    JitEnvironment(InterpreterCodeImpl&);

    ~JitEnvironment();

    void execute();

};

}

#endif // jit_hpp_INCLUDED
