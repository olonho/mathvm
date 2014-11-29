#ifndef _NATIVE_BUILDER_H
#define _NATIVE_BUILDER_H

#include "mathvm.h"
#include "asmjit/asmjit.h"

namespace native {

class Builder {
    Builder() {}
public:
    static void* build(const mathvm::Signature& signature, const void* address);
};

}

#endif // _NATIVE_BUILDER_H
