#include "mathvm.h"
#include "printer_translator.h"

namespace mathvm {

Translator* Translator::create(const string& impl) {
    if (impl == "" || impl == "intepreter") {
        return new BytecodeTranslatorImpl();
    }
    //    if (impl == "jit") {
    //        //return new MachCodeTranslatorImpl();
    //    }
    if (impl == "printer") {
        return new AstPrinterTranslatorImpl();
    }
    assert(false);
    return 0;
}
}
