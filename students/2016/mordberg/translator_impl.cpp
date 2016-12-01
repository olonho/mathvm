#include <iostream>

#include "mathvm.h"
#include "print_translator.h"

namespace mathvm {

Translator* Translator::create(const string& impl) {
    if (impl == "" || impl == "printer") {
        return new PrintTranslator(std::cout);
    } 
    if (impl == "intepreter") {
        // return new BytecodeTranslatorImpl();
        return 0;
    }
    if (impl == "jit") {
        // return new MachCodeTranslatorImpl();
        return 0;
    }
    assert(false);
    return nullptr;
}

}
