#include "mathvm.h"
#include "ast_printer.hpp"

Translator *Translator::create(const string & impl) {
    if (impl == "printer") {
        return new AstPrinter;
    } else {
        return NULL;
    }
}
