#pragma once

#include <vector>
#include <string>

#include "../../../../../libs/asmjit/base/runtime.h"

namespace mathvm {
    using namespace asmjit;
    using namespace asmjit::x86;

    struct Runtime {
        JitRuntime jitRuntime;
        std::vector<std::string> stringPool;

        const char* const PRINT_INT_FORMAT = "%ld";
        const char* const PRINT_DOUBLE_FORMAT = "%lf";
        const char* const PRINT_STRING_FORMAT = "%s";

    };
}