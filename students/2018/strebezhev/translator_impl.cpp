#include <mathvm.h>
#include "include/source_translator_impl.h"
#include "include/bytecode_translator_impl.h"

namespace mathvm {

// Implement me!
  Translator *Translator::create(const string &impl) {
    if (impl == "" || impl == "interpreter") {
      return new BytecodeTranslatorImpl();
    }
    if (impl == "jit") {
      //return new MachCodeTranslatorImpl();
    }
    if (impl == "printer") {
      return new SourceTranslatorImpl();
    }
    return nullptr;
  }

}  // namespace mathvm
