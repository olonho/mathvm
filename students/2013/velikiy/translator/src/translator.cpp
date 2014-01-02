#include "mathvm.h"
#include "bytecodeTranslator.h"

namespace mathvm {
    Translator* Translator::create(const string& impl) {
        if (impl == "" || impl == "intepreter") {
            return new BytecodeTranslator();
        }
        if (impl == "jit") {
            //return new MachCodeTranslatorImpl();
        }
        assert(false);
        return 0;
    }
}
