#include "../../../include/mathvm.h"
#include "../../../vm/parser.h"
#include "../../../include/visitors.h"
#include "PrinterTranslator.h"

namespace mathvm {

// Implement me!
Translator* Translator::create(const string& impl) {
    if (impl == "" || impl == "intepreter") {
        return new BytecodeTranslatorImpl();
    }
    if (impl == "jit") {
        cout << "MachCode Translator isn't implemented yet!" << endl;
        //return new MachCodeTranslatorImpl();
    }

    if (impl == "printer") {
        return new PrinterTranslator();
    }

    assert(false);
    return 0;
}

}
