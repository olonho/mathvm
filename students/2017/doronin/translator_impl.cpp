#include "mathvm.h"
#include "parser.h"
#include "visitors.h"
#include "printer_translator_impl.h"

namespace mathvm {


Translator* Translator::create(const string& impl) {
    if (impl == "printer") {
        return new PrinterTranslatorImpl();
    }
    if (impl == "" || impl == "intepreter") {
        //return new BytecodeTranslatorImpl();
        return 0;
    }
    if (impl == "jit") {
        //return new MachCodeTranslatorImpl();
    }
    assert(false);
    return 0;
}

}
