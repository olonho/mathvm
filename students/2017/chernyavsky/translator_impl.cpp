#include "bytecode_translator.h"
#include "prettyprint_translator.h"

namespace mathvm {

    Translator* Translator::create(const string& impl) {
        if (impl.empty() || impl == "interpreter") {
            return new BytecodeTranslatorImpl();
        } else if (impl == "printer") {
            return new PrettyPrintTranslatorImpl();
        } else if (impl == "jit") {
            //return new MachCodeTranslatorImpl();
        }

        assert(false);
        return nullptr;
    }

}
