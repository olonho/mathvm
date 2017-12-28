#pragma once

#include <cstdint>
#include <string>
#include <map>
#include "includes.h"
#include "stack.h"
#include <dlfcn.h>
#include <cstring>
#include <sstream>

namespace mathvm {

    struct native_caller {
        native_caller(const std::map<uint16_t, std::pair<std::string, std::vector<mathvm::VarType>>> &nativeFunctions) :
                nativeFunctions(nativeFunctions) {}


        void call(uint16_t function_id, mathvm::Stack &stack, const std::vector<std::string> &constantStrings,
                  std::map<uint16_t, char *> &dynamicStrings);

    private:
        using signature_t = std::vector<mathvm::VarType>;
        const std::map<uint16_t, std::pair<std::string, signature_t>> nativeFunctions;
    };
}
