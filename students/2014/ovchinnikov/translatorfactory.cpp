#include "mathvm.h"
#include "astprinter.hpp"
#include "bytecodetranslator.hpp"

using namespace mathvm;

Translator *Translator::create(const string & impl) {
    if (impl == "translator" || impl == "") {
        return new BytecodeTranslator();
    } else if (impl == "printer") {
        return new AstPrinter();
    } else {
        return 0;
    }
}
