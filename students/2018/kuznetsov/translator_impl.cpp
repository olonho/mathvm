#include <mathvm.h>
#include "astprinter/ast_printer.h"

namespace mathvm {

    Translator* Translator::create(const string& impl) {
        if (impl == "" || impl == "intepreter") {
            //return new BytecodeTranslatorImpl();
        }
        if (impl == "jit") {
            //return new MachCodeTranslatorImpl();
        }
        if (impl == "printer") {
            return new source_translator_impl();
        }
        return nullptr;
    }

}  // namespace mathvm
