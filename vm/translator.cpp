#include "mathvm.h"
#include "parser.h"
#include "visitors.h"

namespace mathvm {

// Implement me!
Translator* Translator::create(const string& impl) {
    if (impl == "" || impl == "intepreter") {
        //return new BytecodeTranslatorImpl();
        return 0;
    }
    assert(false);
    return 0;
}

}
