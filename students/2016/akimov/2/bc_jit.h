#pragma once

#include <mathvm.h>
#include <asmjit/asmjit.h>
#include <memory>

namespace mathvm {

class BytecodeJITHelper {
    vector<unique_ptr<asmjit::JitRuntime>> _runtimes;
public:
    void* compileNative(const Signature* signature, const void* ptr);
};

}
