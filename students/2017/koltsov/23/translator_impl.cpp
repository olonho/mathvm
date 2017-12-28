#include "mathvm.h"


namespace mathvm {

Translator* Translator::create(const string& impl) {
    (void)impl;
    return new BytecodeTranslatorImpl();
}

}
