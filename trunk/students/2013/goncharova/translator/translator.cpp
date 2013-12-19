#include "mathvm.h"
#include "parser.h"
#include "visitors.h"
#include "BytecodeTranslatorImpl.h"

namespace mathvm {

// Implement me!
Translator* Translator::create(const string& impl) {
    if (impl == "") {
        return new TranslatorImpl();
    }
    if (impl == "intepreter") {
	//return interpreter
    }
    if (impl == "jit") {
        //return new MachCodeTranslatorImpl();
    }
    assert(false);
    return 0;
}

}
