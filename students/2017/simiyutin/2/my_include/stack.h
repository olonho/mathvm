#pragma once

#include "includes.h"

namespace mathvm {
    struct Stack : mathvm::Bytecode {
        int64_t getInt64() {
            return getTyped<int64_t>();
        }
        uint16_t getUInt16() {
            return getTyped<uint16_t>();
        }

        double getDouble() {
            return getTyped<double>();
        }

        template <typename T>
        T getTyped() {
            if (_data.size() < sizeof(T)) {
                std::cout << "stack has less bytes than needed for requested type" << std::endl;
                std::cout << "cur stack size: " << _data.size() << std::endl;
                std::cout << "needed: " << sizeof(T) << std::endl;
                exit(42);
            }

            T result = mathvm::Bytecode::getTyped<T>((uint32_t)(_data.size() - sizeof(T)));
            for (size_t i = 0; i < sizeof(T); ++i) {
                _data.pop_back();
            }
            return result;
        }
    };
}
