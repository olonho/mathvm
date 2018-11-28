#include <mathvm.h>
#include "astprinter/ast_printer.h"
#include "bytecode/bytecode_interpreter.h"

namespace mathvm {

    Translator* Translator::create(const string& impl) {
        if (impl == "" || impl == "interpreter") {
//            return new BytecodeTranslatorImpl();
			return new bytecode_interpreter_impl();
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
