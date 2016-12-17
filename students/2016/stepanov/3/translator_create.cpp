#include "../../../../include/mathvm.h"
#include "BytecodeRFTranslator.h"
#include "JTranslator.h"

namespace mathvm {

    Translator *Translator::create(const string &impl) {
        if (impl == "printer") {
            return NULL;
        }
        else if (impl == "jit") {
            return new JTranslator();
        }

        //return new BytecodeRFTranslator();
        return new JTranslator();
    }

}

