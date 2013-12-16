#include "mathvm.h"

#include "machcode.h"
#include "jitcompiler.h"

#include <stdio.h>
#include <stdlib.h>

using namespace mathvm;

int main(int argc, char *argv[]) {
    if(argc < 2) {
        std::cout << "Usage: " << argv[0] << " <program.mvm> [debug]" << std::endl;
        return -1;
    }

    char *inCode = loadFile(argv[1]);
    if(!inCode) {
        std::cout << "Unable to open file: " << argv[1] << std::endl;
        return -1;
    }

    JITCompiler t(argc > 2);
    AsmJitCodeImpl *outCode = 0;
    Status *res = t.translate(inCode, (Code**)&outCode);
    if(res && res->isError()) {
        std::cout << "Compilation error: "  << res->getError();
        if(res->getPosition() != 0) {
            uint32_t line, offset;
            positionToLineOffset(inCode, res->getPosition(), line, offset);
            std::cout << " at line " << line << " pos " << offset;
        }
        std::cout << std::endl;
        return -1;
    }

    try {
        std::vector<Var*> vars;
        outCode->execute(vars);
    } catch(...) {
        std::cout << "execution error" << std::endl;
    }

    std::cout.flush();
    return 0;
}
