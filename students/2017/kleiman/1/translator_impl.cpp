#include "include/ast_printing.h"
#include <iostream>

namespace mathvm {
    Translator* Translator::create(const string& impl) 
    {
        if (impl == "" || impl == "intepreter") {
            //return new BytecodeTranslatorImpl();
            return 0;
        }
        if (impl == "jit") {
            //return new MachCodeTranslatorImpl();
        }
        if (impl == "printer") {
            return new ProgramTranslatorImpl();
        }
        assert(false);
        return 0;
    }

}