#include "mathvm.h"
#include "parser.h"
#include "visitors.h"
#include "pretty_printer.h"
#include "bytecode_translator.h"

#include <iostream>
#include <stdarg.h>

namespace mathvm {

Translator* Translator::create(const string& impl) {
    if (impl == "printer")
        return new AstTranslatorImpl();
    if (impl == "" || impl == "intepreter")
        return new BytecodeTranslatorImpl();

    std::cout << "option '" << impl << "' is not supported" << "'\n";
    assert(false);

    return nullptr;
}

}
