#include <stdexcept>

#include "mathvm.h"
#include "m_translator.h"

using namespace mathvm;

Translator *Translator::create(const string &impl) {
    if (impl == "" || impl == "interpreter") {
        return new TranslatorToBytecode();
    }
    if (impl == "jit") {
        // not done
    }
    assert(false);

    return nullptr;
}

