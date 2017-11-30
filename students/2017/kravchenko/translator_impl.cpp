#include "mathvm.h"
#include "parser.h"
#include "visitors.h"
#include "pretty_printer.h"

#include <iostream>
#include <stdarg.h>

namespace mathvm {

Translator* Translator::create(const string& impl) {
    if (impl == "printer" || impl == "")
        return new AstTranslatorImpl();

    std::cout << "Only AST translator supported for now, you asked for '" << impl << "'\n";
    assert(false);

    return nullptr;
}

}
