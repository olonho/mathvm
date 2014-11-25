#include "mathvm.h"
#include "parser.h"
#include "visitors.h"

namespace mathvm {

Translator* Translator::create(const string& impl) {
    if (impl == "" || impl == "interpreter") {
        return new BytecodeTranslatorImpl();
    }
    if (impl == "jit") {
        //return new MachCodeTranslatorImpl();
    }
    assert(false);
    return 0;
}

}
