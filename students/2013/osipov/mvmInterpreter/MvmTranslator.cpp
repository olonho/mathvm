
#include "parser.h"
#include "MvmTranslator.h"

using namespace mathvm;

MvmTranslator::MvmTranslator() {}


Status* MvmTranslator::translate(const string& program, Code**code) {
    Parser parser;
    if (Status * s = parser.parseProgram(program)) {
        return s;
    }

    MvmBytecode* bcode = new MvmBytecode();
    *code = bcode;
    return 0;
}
