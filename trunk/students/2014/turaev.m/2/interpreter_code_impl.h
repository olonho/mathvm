#include "mathvm.h"

namespace mathvm {
class InterpreterCodeImpl: public Code {
    virtual Status* execute(vector<Var*>& vars) {
        return 0;
    }   
};
}