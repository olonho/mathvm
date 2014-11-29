#ifndef _BYTECODE_INTERPRETER_H
#define _BYTECODE_INTERPRETER_H

#include "interpreter_stack.h"
#include "native_builder.h"

namespace mathvm {

class InterpreterCodeImpl: public Code {
    vector<uint32_t> _functionStackSize;

    BytecodeFunction* getFunction(uint16_t id) const {
        return (BytecodeFunction*) functionById(id);
    }

  public:

    virtual Status* execute(vector<Var*>&);

    uint16_t functionsNumber() const {
        return _functionStackSize.size();
    }

    uint16_t addFunction(TranslatedFunction* f) {
        _functionStackSize.push_back(0);
        return Code::addFunction(f);
    }

    uint32_t stackSize(uint16_t id) const {
        return _functionStackSize[id];
    }

    void onStackGrow(uint16_t id, uint32_t newSize) {
        if (newSize > stackSize(id))
            _functionStackSize[id] = newSize;
    }

    uint16_t getNativeId(const string& name) {
        Signature hack; // _nativeById is private :(
        return Code::makeNativeFunction(name, hack, NULL);
    }

    uint16_t makeNativeFunction(const string& name,
                                const Signature& signature,
                                const void* address) {
        void* asmjitted = native::Builder::build(signature, address);
        return Code::makeNativeFunction(name, signature, asmjitted);
    }
};

}

#endif // _BYTECODE_INTERPRETER_H
