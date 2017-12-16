#include "mathvm.h"
#include "include/print_translator.h"
#include "include/bytecode_translator.h"

namespace mathvm {
    Translator *Translator::create(const string &impl) {
        if (impl == "" || impl == "interpreter") {
            return new BytecodeTranslator();
        } else if (impl == "printer") {
            return new AstPrintTranslator();
        } else if (impl == "jit") {
            //return new MachCodeTranslatorImpl();
        }
        assert(false);
        return 0;
    }
}

