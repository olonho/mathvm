#include "bytecode_translator.h"

namespace mathvm {

    Translator* Translator::create(const string& impl) {
        if (impl == "" || impl == "intepreter") {
            return new BytecodeTranslator();
        }
        if (impl == "jit") {
            //return new MachCodeTranslatorImpl();
            return 0;
        }
        if (impl == "printer") {
            //return new PrinterTranslatorImpl(std :: cout);
            return 0;
        }
        assert(false);
        return 0;
    }

}
