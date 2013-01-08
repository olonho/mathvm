#include "mathvm.h"
#include "parser.h"
#include "visitors.h"

#include "bcg.h"

namespace mathvm {

// Implement me!
Translator* Translator::create(const string& impl) {
    if (impl == "" || impl == "intepreter") {
        //return new BytecodeTranslatorImpl();
        return new BytecodeTranslator();
    }
    if (impl == "jit") {
        //return new MachCodeTranslatorImpl();
    }
    assert(false);
    return 0;
}

}
