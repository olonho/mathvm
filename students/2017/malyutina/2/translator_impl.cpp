#include "../../../../include/mathvm.h"
#include "include/ast_to_bytecode_translator.h"

using namespace mathvm;

Translator *Translator::create(const string &impl) {
    if (impl == "translator") {
        return new ast_to_bytecode_translator();
    } else {
        return NULL;
    }
}
