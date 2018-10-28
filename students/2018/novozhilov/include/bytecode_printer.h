#ifndef VIRTUAL_MACHINES_BYTECODE_PRINTER_H
#define VIRTUAL_MACHINES_BYTECODE_PRINTER_H

#include <mathvm.h>
#include <iostream>

namespace mathvm {
    void printBytecode(Code *code, ostream &out);
}

#endif //VIRTUAL_MACHINES_BYTECODE_PRINTER_H
