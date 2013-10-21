#include "mathvm.h"

namespace mathvm {

class CodeImpl : public Code {
public:
    CodeImpl() {}
    virtual ~CodeImpl() {}

    /**
     * Execute this code with passed parameters, and update vars
     * in array with new values from topmost scope, if code says so.
     */
    virtual Status* execute(vector<Var*>& vars);
};

}