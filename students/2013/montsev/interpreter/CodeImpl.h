#ifndef CODE_IMPL_H
#define CODE_IMPL_H

#include "parser.h"
#include <stack>

namespace mathvm {

class CodeImpl : public Code {
private: // structs

    union Val {
        int64_t ival;
        double dval;
        uint16_t sval;
    };

private: // typedefs
    typedef stack<Val> Stack;
    typedef vector<Val> VariableStack;
    typedef stack<VariableStack> ContextStack;

public:
    CodeImpl(): _fid(0), _ip(0) {}
    virtual ~CodeImpl() {}

    virtual Status* execute();
         
    virtual Status* execute(vector<Var*>& vars);

private: // methods

    // init
    void initialize(); 

    // checkers

    void validate();

    // runners

    void executeMachine();

private: // fields

    uint16_t _fid;
    uint32_t _ip;

    Bytecode* _bc;

    Stack _computationStack;
    vector<ContextStack> _functions;
    stack<pair<uint16_t, uint32_t> > _functionStackTrace;

    vector<Val> _dReg;
    vector<Val> _iReg;
    vector<Val> _sReg;
};

}

#endif // CODE_IMPL_H