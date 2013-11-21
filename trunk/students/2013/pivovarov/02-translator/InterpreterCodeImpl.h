#include "mathvm.h"

#include <string>
#include <vector>
using std::string;
using std::vector;

#include <stdexcept>

#include <tr1/memory>
using std::tr1::shared_ptr;

namespace mathvm {

const string type2str(VarType const & type);
const string int2str(long val);

class Bytecode_ : public Bytecode {
public:
    uint8_t * getData() {
        return _data.data();
    }
};

class BytecodeFunction_ : public TranslatedFunction {
    Bytecode_ bytecode_;

public:
    BytecodeFunction_(const string & name, const Signature & signature) :
        TranslatedFunction(name, signature) {
    }

    Bytecode_ * bytecode() {
        return &bytecode_;
    }

    virtual void disassemble(ostream& out) const {
        bytecode_.dump(out);
    }
};

class NativeFunction_ : public TranslatedFunction {
    void const * const pointer;
    uint16_t intParams_;
    uint16_t doubleParams_;

public:
    NativeFunction_(string const & name, const Signature& signature, void const * const pointer) :
        TranslatedFunction(name, signature), pointer(pointer), intParams_(0), doubleParams_(0) {
            for (uint16_t i = 0; i < parametersNumber(); ++i) {
                switch (parameterType(i)) {
                    case VT_INT:
                    case VT_STRING:
                        intParams_++;
                        break;
                    case VT_DOUBLE:
                        doubleParams_++;
                        break;
                    default:
                        throw logic_error("Bad native parameter type: " + type2str(parameterType(i)) );
                }
            }
            if (intParams_ > 6 || doubleParams_ > 8) {
                throw logic_error("Too much parameters in native call: " + name);
            }
    }

    virtual void disassemble(ostream & out) const {
        out << "[Native]" << pointer << endl;
    }

    void const * const ptr() {
        return pointer;
    }

    uint16_t intParams() {
        return intParams_;
    }

    uint16_t doubleParams() {
        return doubleParams_;
    }
};

struct FunctionData {
    uint16_t stack_size;
    uint16_t local_vars;
    BytecodeFunction_ * fun;
    NativeFunction_ * native_fun;

    FunctionData() {}

    FunctionData(BytecodeFunction_ * fun)
        : stack_size(0), local_vars(0), fun(fun), native_fun(NULL) {}

    FunctionData(NativeFunction_ * fun)
        : stack_size(-1), local_vars(-1), fun(NULL), native_fun(fun) {}

    bool isNative() {
        return native_fun != NULL;
    }
};

class InterpreterCodeImpl : public Code {
    vector<FunctionData> funsData;
public:
    InterpreterCodeImpl() {
        makeStringConstant("");
    }

    virtual ~InterpreterCodeImpl() {
        for (uint32_t i = 0; i < funsData.size(); ++i) {
            if (funsData[i].isNative()) {
                delete funsData[i].native_fun;
            }
        }
    }

    void addFunctionData(uint16_t id, FunctionData data) {
        if (funsData.size() <= id) {
            funsData.resize(id + 1);
        }
        funsData[id] = data;
    }

    FunctionData * getFunctionData(uint16_t id) {
        if (id >= funsData.size()) {
            throw logic_error("Unknown functionData: " + int2str(id));
        }
        return &funsData[id];
    }

    Status * execute(vector<Var*> & vars);

    Status * execute();
};

}