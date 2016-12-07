#include <iostream>

#include "mathvm.h"
#include "print_translator.h"
#include "translator_impl.h"

namespace mathvm {

const std::string CMD_PRINTER = "printer";
const std::string CMD_TRANSLATOR = "translator";
const std::string CMD_INTERPRETER = "interpreter";
const std::string CMD_JIT = "jit";

Translator* Translator::create(const string& impl) {
    if (impl == CMD_PRINTER) {
        return new PrintTranslator(std::cout);
    } 
    if (impl == "" || impl == CMD_INTERPRETER || impl == CMD_TRANSLATOR) {
        return new BytecodeTranslatorImpl();
    }
    if (impl == CMD_JIT) {
        // return new MachCodeTranslatorImpl();
        return nullptr;
    }
    assert(false);
    return nullptr;
}

}
