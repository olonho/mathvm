#include "mathvm.h"
#include "ast_to_bytecode_translator.h"

using namespace mathvm;

Translator* Translator::create(const string& impl) {
  if (impl == "translator") {
    return new AstToBytecodeTranslator();
  } else {
    return NULL;
  }
}
