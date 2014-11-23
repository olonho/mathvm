#ifndef JIT_BUILDER
#define JIT_BUILDER
#include "mathvm.h"
#include "common.h"
#include "asmjit/asmjit.h"

namespace mathvm {

    class JitBuilder {
    public:

        void *buildNativeProxy(Signature const & signature, const void * addr);

        static JitBuilder & instance() {
            return instance_;
        }

    private:
        JitBuilder() {}
        asmjit::JitRuntime runtime_;
        static JitBuilder instance_;
    };

}
#endif //JIT_BUILDER
