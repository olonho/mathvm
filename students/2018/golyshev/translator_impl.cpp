#include <mathvm.h>
#include "pretty_print_translator_impl.hpp"

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
       return new PrettyPrintTranslatorImpl(cout);
   }
   return nullptr;
}

}  // namespace mathvm
