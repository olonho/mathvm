#include "my_utils.h"

string instructionToString(Instruction insn) {
#define BC_NAME(b, d, l) #b,
    const char* names[] = {
        FOR_BYTECODES(BC_NAME)
        ""
    };
#undef BC_NAME
    if(insn >= BC_LAST) {
        return "binary";
    }
    return names[insn];
}

void DEBUG_MSG(string const& msg) {
//    cout << "#DEBUG: " << msg << '\n';
}

void DEBUG_MSG(Bytecode* bc) {
//    cout << "#DEBUG: ==bytecode start==\n";
//    for (size_t i = 0; i != bc->length(); ++i) {
//        cout << "#DEBUG: " << instructionToString(bc->getInsn(i)) << endl;
//    }
}

void DEBUG_MSG(vector<VarType> const& typesStack) {
//    DEBUG_MSG("== Types Stack ==");
//    DEBUG_MSG("size: " + to_string(typesStack.size()));
//    for (size_t i = 0; i != typesStack.size(); ++i) {
//        cout << typeToName(typesStack.at(i)) << endl;
//    }
//    DEBUG_MSG("== =========== ==");
}
