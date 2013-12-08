#include "parser.h"

namespace mathvm {

class CodeImpl : public Code {
public:
    CodeImpl() {}
    virtual ~CodeImpl() {}

    virtual Status* execute(vector<Var*>& vars) {
        // TODO Implement me
        return 0;
    }
};

}

