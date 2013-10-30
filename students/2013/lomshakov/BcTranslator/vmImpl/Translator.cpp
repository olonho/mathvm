#include "mathvm.h"

namespace mathvm {


Translator* Translator::create(const string& impl) {
    if (impl == "" || impl == "intepreter") {
      return new BytecodeTranslatorImpl();

    }
    if (impl == "jit") {
        //return new MachCodeTranslatorImpl();
    }

    assert(false);
    return 0;
}

}
