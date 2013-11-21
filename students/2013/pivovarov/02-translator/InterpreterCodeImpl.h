#include "mathvm.h"

#include <string>
#include <vector>
using std::string;
using std::vector;

#include <stdexcept>

#include <tr1/memory>
using std::tr1::shared_ptr;

namespace mathvm {

const string int2str(long val);

class Bytecode_ : public Bytecode {
public:
    uint8_t* getData() {
        return _data.data();
    }
};

class BytecodeFunction_ : public TranslatedFunction {
    Bytecode_ _bytecode;

public:
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

class NativeFunction_ : public TranslatedFunction {
    void const * const pointer;

public:
    NativeFunction_(const string& name, const Signature& signature, void const * const pointer) :
        TranslatedFunction(name, signature), pointer(pointer) {
    }

    virtual void disassemble(ostream& out) const {
        out << "[Native]" << pointer << endl;
    }

    void const * const ptr() {
        return pointer;
    }
};

struct FunctionData {
    FunctionData() {}
    FunctionData(uint16_t stack_size, BytecodeFunction_ * fun)
        : stack_size(stack_size), fun(fun) {}

    FunctionData(NativeFunction_ * fun)
        : stack_size(-1), native_fun(fun) {}

    uint16_t stack_size;
    BytecodeFunction_ * fun;
    shared_ptr<NativeFunction_> native_fun;
};

class InterpreterCodeImpl : public Code {
    vector<FunctionData> funsData;
public:
    InterpreterCodeImpl() {
        makeStringConstant("");
    }
    virtual ~InterpreterCodeImpl() {}

    void addFunctionData(uint16_t id, FunctionData data);
    FunctionData const & getFunctionData(uint16_t id);

    Status * execute(vector<Var*> & vars);

    Status * execute();
};

}