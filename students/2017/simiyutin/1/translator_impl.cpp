#include "mathvm.h"
#include "include/ast_printer_translator.h"

namespace mathvm {

    Translator* Translator::create(const string& impl) {
        if (impl == "printer") {
            return new AstPrinterTranslatorImpl();
        }
        if (impl == "" || impl == "intepreter") {
            //return new BytecodeTranslatorImpl();
            return nullptr;
        }
        if (impl == "jit") {
            //return new MachCodeTranslatorImpl();
        }
        assert(false);
        return nullptr;
    }

}