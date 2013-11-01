#include "mathvm.h"

#include <string>
#include <vector>
using std::string;
using std::vector;

#include <stdexcept>

namespace mathvm {

class Bytecode_ : public Bytecode {
public:
    uint8_t* getData() {
        return _data.data();
    }
};

class BytecodeFunction_ : public TranslatedFunction {
    Bytecode_ _bytecode;

public:
    BytecodeFunction_(AstFunction* function) :
        TranslatedFunction(function) {
    }

    BytecodeFunction_(const string& name, const Signature& signature) :
        TranslatedFunction(name, signature) {
    }

    Bytecode_* bytecode() {
        return &_bytecode;
    }

    virtual void disassemble(ostream& out) const {
        _bytecode.dump(out);
    }
};

struct FunctionData {
    FunctionData() {}
    FunctionData(uint16_t stack_size, BytecodeFunction_ * fun)
        : stack_size(stack_size), fun(fun) {}

    uint16_t stack_size;
    BytecodeFunction_ * fun;
};

class InterpreterCodeImpl : public Code {
    vector<FunctionData> funsData;
public:
    InterpreterCodeImpl() {}
    virtual ~InterpreterCodeImpl() {}

    void addFunctionData(uint16_t id, FunctionData data);
    FunctionData const & getFunctionData(uint16_t id);

    Status * execute(vector<Var*> & vars);

    Status * execute();
};

}