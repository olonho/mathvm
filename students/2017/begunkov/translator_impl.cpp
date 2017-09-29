#include "mathvm.h"
#include "visitors.h"
#include "source_translator.hpp"

namespace mathvm {

    Translator* Translator::create(const string& impl) {
        if (impl == "printer") {
            return new SourceTranslatorImpl();
        }

        if (impl == "" || impl == "intepreter") {
            //return new BytecodeTranslatorImpl();
            return 0;
        }
        if (impl == "jit") {
            //return new MachCodeTranslatorImpl();
            return 0;
        }
        return 0;
    }

}

