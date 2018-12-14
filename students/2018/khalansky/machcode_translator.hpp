#ifndef machcode_translator_hpp_INCLUDED
#define machcode_translator_hpp_INCLUDED

#include <mathvm.h>
#include "jit.hpp"

namespace mathvm {

class MachCodeImpl : public Code {

    JitEnvironment env;

public:

    MachCodeImpl(InterpreterCodeImpl& code);

    virtual Status* execute(vector<Var*>& vars);
};

}

#endif // machcode_translator_hpp_INCLUDED

