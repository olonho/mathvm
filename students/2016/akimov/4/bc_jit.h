#pragma once

#include <mathvm.h>
#include <asmjit/asmjit.h>
#include <memory>

namespace mathvm {

class InterpreterCodeImpl : public Code {
public:
    InterpreterCodeImpl() : _runtime(new asmjit::JitRuntime()) {}
    ~InterpreterCodeImpl() {}
    virtual Status* execute(vector<Var*>& vars) override;

    BytecodeFunction* functionByName(const string& name) {
        return dynamic_cast<BytecodeFunction*>(Code::functionByName(name));
    }

    BytecodeFunction* functionById(uint16_t id) {
        return dynamic_cast<BytecodeFunction*>(Code::functionById(id));
    }

    void* processNativeFunction(const Signature* signature, const void* ptr);

private:
    asmjit::JitRuntime* getRuntime() {
        return _runtime.get();
    }
    void processFunction(BytecodeFunction* function);

    unique_ptr<asmjit::JitRuntime> _runtime;
    vector<uint16_t> _scopeSize;
    void** _scopes;
    void** _functions;
};

}
