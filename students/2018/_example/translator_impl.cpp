#include <mathvm.h>

namespace mathvm {

// Implement me!
Translator* Translator::create(const string& impl) {
   if (impl == "" || impl == "intepreter") {
       //return new BytecodeTranslatorImpl();
   }
   if (impl == "jit") {
       //return new MachCodeTranslatorImpl();
   }
   if (impl == "printer") {
       //return new SourceTranslatorImpl();
   }
   return nullptr;
}

}  // namespace mathvm
