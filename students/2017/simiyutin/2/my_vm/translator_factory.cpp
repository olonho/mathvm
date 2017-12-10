#include "../my_include/astprinter.h"

namespace mathvm {

    Translator* Translator::create(const string& impl) {
        if (impl == "printer") {
            return new AstPrinter();
        }
        if (impl == "" || impl == "intepreter") {
            return new BytecodeTranslatorImpl();
        }
        if (impl == "jit") {
            //return new MachCodeTranslatorImpl();
        }
        assert(false);
        return nullptr;
    }

}