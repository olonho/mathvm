#include "../../../../include/mathvm.h"
#include "BytecodeRFTranslator.h"

namespace mathvm {

    Translator *Translator::create(const string &impl) {
        if (impl == "printer") {
            return NULL;
        }

        return new BytecodeRFTranslator();
    }

}

