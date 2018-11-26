#ifndef VIRTUAL_MACHINES_MYINTERPRETERCODEIMPL_H
#define VIRTUAL_MACHINES_MYINTERPRETERCODEIMPL_H

#include "mathvm.h"

namespace mathvm {
    class InterpreterCodeImpl : public Code {
    public:
        Status *execute(vector<Var *> &vars);

    };

    class StackValue {
    private:
        double dVal;
        int64_t iVal;
        const string sVal;
    public:
        StackValue(double dVal);

        StackValue(int64_t iVal);

        StackValue(const string &sVal);

        const double &getDVal() const;

        const int64_t &getIVal() const;

        const string &getSVal() const;

    };
}


#endif //VIRTUAL_MACHINES_MYINTERPRETERCODEIMPL_H
