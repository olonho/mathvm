#include "PrinterTranslator.h"

namespace mathvm {

Translator* Translator::create(const string& impl) {
    if (impl == "printer") {
        return new PrinterTranslator();
    }

    return nullptr;
}

}

