#include "printer_translator.h"
#include "bytecode_translator.h"

namespace mathvm {

Translator* Translator::create(const string& impl) {
    if (impl == "printer") {
        return new PrinterTranslator();
    }

    if (impl == "translator") {
        return new BytecodeTranslatorImpl();
    }

    return nullptr;
}

}

