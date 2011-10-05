#ifndef MYCODE_H
#define MYCODE_H

#include "mathvm.h"

struct MyCode : mathvm::Code {
private:
    mathvm::Bytecode myBytecode;
public:
    virtual mathvm::Status* execute(std::vector<mathvm::Var*> vars) {
        return NULL;
    }

    void setBytecode(mathvm::Bytecode const & bytecode) {
        myBytecode = bytecode;
    }

    mathvm::Bytecode const & getBytecode() const {
        return myBytecode;
    }
};

#endif // MYCODE_H
