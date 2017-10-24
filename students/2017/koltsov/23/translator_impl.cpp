#include "mathvm.h"


namespace mathvm {

Translator* Translator::create(const string& impl) {
    return new BytecodeTranslatorImpl();
}

}
