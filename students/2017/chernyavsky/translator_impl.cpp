#include "pretty_print.h"

namespace mathvm {

    Translator *Translator::create(const string &impl) {
        if (impl.empty() || impl == "intepreter") {
            //return new BytecodeTranslatorImpl();
        } else if (impl == "printer") {
            return new PrettyPrintTranslatorImpl();
        } else if (impl == "jit") {
            //return new MachCodeTranslatorImpl();
        }

        assert(false);
        return nullptr;
    }

}
