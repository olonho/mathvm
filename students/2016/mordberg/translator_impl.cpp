#include <iostream>

#include "mathvm.h"
#include "print_translator.h"

namespace mathvm {

Translator* Translator::create(const string& impl) {
    if (impl == "printer") {
        return new PrintTranslator(std::cout);
    } 
    if (impl == "" || impl == "interpreter") {
        return new BytecodeTranslatorImpl();
    }
    if (impl == "jit") {
        // return new MachCodeTranslatorImpl();
        return nullptr;
    }
    assert(false);
    return nullptr;
}

}
