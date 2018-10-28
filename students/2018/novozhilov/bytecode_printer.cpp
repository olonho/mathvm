#include "include/bytecode_printer.h"

using namespace mathvm;

void mathvm::printBytecode(Code *code, ostream &out) {
    Code::FunctionIterator functionIterator(code);
    while (functionIterator.hasNext()) {
        auto *function = dynamic_cast<BytecodeFunction*>(functionIterator.next());
        out << "function " << function->name() << endl;
        function->bytecode()->dump(out);
        out << endl << endl;
    }
}
