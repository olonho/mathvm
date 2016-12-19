#include "PrintTranslatorImpl.h"

namespace mathvm {

Translator *Translator::create(const string& impl) {
    if (impl == "" || impl == "intepreter") {
        return new BytecodeTranslatorImpl();
    }
    if (impl == "jit") {
        //return new MachCodeTranslatorImpl();
        return 0;
    }
    if (impl == "printer") {
        return new PrintTranslatorImpl();
    }
    assert(false);
    return 0;
}

}
