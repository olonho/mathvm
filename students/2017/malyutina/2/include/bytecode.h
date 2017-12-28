#ifndef RESULT_BYTECODE_H__
#define RESULT_BYTECODE_H__

#include "../../../../../include/mathvm.h"

#include <string>

using namespace mathvm;

class byte_code : public Code {
public:
    virtual Status *execute(vector<Var *> &vars) {
        return Status::Ok();
    }

    virtual void disassemble(ostream &out = cout, FunctionFilter *filter = 0) {
        const std::string topName = "<top>";
        auto topFunction = functionByName(topName);
        topFunction->disassemble(out);
    }
};

#endif
