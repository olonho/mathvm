#pragma once


#include <elf.h>

namespace mathvm {
    struct NativeRepr {
        struct Function {
            const uint16_t id;
            size_t codeLength;
            char* data;
        };

    };
}