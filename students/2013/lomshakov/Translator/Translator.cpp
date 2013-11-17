#include "mathvm.h"
#include "BCTranslatorImpl.h"

namespace mathvm {


Translator* Translator::create(const string& impl) {
    if (impl == "" || impl == "intepreter") {
      return new BCTranslatorImpl();

    }
    if (impl == "jit") {
        //return new MachCodeTranslatorImpl();
    }

    assert(false);
    return 0;
}

}
