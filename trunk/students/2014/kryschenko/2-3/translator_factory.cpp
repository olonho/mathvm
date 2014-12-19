#include <stdexcept>

#include "mathvm.h"
#include "bytecode_translator.hpp"

using namespace mathvm;

Translator *Translator::create(const string &impl) {
    if (impl == "" || impl == "translator") {
        return new BytecodeTranslator();
    }
    if (impl == "jit") {
        throw new std::runtime_error("JIT is not implemented");
    }
    assert(false);
    return 0;
}

