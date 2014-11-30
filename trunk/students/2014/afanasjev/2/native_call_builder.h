#pragma once

#include "asmjit/asmjit.h"
#include "mathvm.h"

namespace mathvm {

typedef int64_t (*nativeCall)(int64_t*);

class NativeCallBuilder {
public:    
    
    nativeCall makeCall(NativeFunctionDescriptor const & descr);
private:
    int calcNumberOfStackArgs(Signature const & sig);
    static asmjit::JitRuntime runtime;

    static const int maxGpArgs = 6;
    static const int maxXmmArgs = 8;
};

}
