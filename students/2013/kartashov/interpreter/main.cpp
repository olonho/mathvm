#include "bctranslator.h"
#include "bcinterpreter.h"

using namespace mathvm;

int main(int argc, char *argv[]) {
    if(argc != 2) {
        std::cout << "Usage: " << argv[0] << " <program.mvm>" << std::endl;
        return -1;
    }

    char *inCode = loadFile(argv[1]);
    if(!inCode) {
        std::cout << "Unable to open file: " << argv[1] << std::endl;
        return -1;
    }

    BCTranslator t;
    InterpreterCodeImpl *outCode = 0;
    Status *res = t.translate(inCode, (Code**)&outCode);
    if(res && res->isError()) {
        std::cout << "BytecodeTranslator error: "  << res->getError();
        if(res->getPosition() != (uint32_t)-1) {
            uint32_t line, offset;
            positionToLineOffset(inCode, res->getPosition(), line, offset);
            std::cout << " at line " << line << " pos " << offset;
        }
        std::cout << std::endl;
        return -1;
    }

    std::vector<Var*> v;
    outCode->execute(v);

    return 0;
}
