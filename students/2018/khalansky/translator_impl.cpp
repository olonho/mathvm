#include <mathvm.h>
#include "source_translator.hpp"

#include <iostream>

namespace mathvm {

// Implement me!
Translator* Translator::create(const string& impl) {
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
