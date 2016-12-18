#include "mathvm.h"
#include "include/ast_to_bytecode_translator.h"

namespace mathvm {

Translator* Translator::create(const string& impl) {
    if (impl == "translator") {
        return new AstToBytecodeTranslator();
    } else {
        return nullptr;
    }
}   

}
