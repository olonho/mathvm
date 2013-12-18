#include "mathvm.h"
#include "BCTranslatorImpl.h"
#include "MachineCodeTranslatorImpl.h"

namespace mathvm {


Translator* Translator::create(const string& impl) {
    if (impl == "" || impl == "intepreter") {
      return new BCTranslatorImpl();

    }
    if (impl == "jit") {
        return new MachineCodeTranslatorImpl();
    }

    assert(false);
    return 0;
}

}
