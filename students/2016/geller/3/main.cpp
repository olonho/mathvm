#include "../../../../include/mathvm.h"
#include "bytecode_executor.h"
using namespace mathvm;
using namespace std;

int main(int argc, char **argv) {

    string filename;
    if(argc == 2){
        filename = argv[1];
    }else{
        filename  = "/home/wimag/Yandex.Disk/My Stuff/SPBAU/VM/mathvm/tests/additional/fib.mvm";
        filename  = "/home/wimag/Yandex.Disk/My Stuff/SPBAU/VM/mathvm/tests/additional/ackermann.mvm";
    }
    Translator *translator = new BytecodeTranslatorImpl;

    const char *expr = loadFile(filename.c_str());
    if (expr == 0) {
        printf("Cannot read file: %s\n", filename.c_str());
        return 1;
    }


    Code *code = 0;

    Status *translateStatus = translator->translate(expr, &code);
    if (translateStatus->isError()) {
        uint32_t position = translateStatus->getPosition();
        uint32_t line = 0, offset = 0;
        positionToLineOffset(expr, position, line, offset);
        printf("Cannot translate expression: expression at %d,%d; "
                       "error '%s'\n",
               line, offset,
               translateStatus->getErrorCstr());
    } else {
        code->disassemble();
        bytecode_executor((InterpreterCodeImpl *) code).execute();
    }
    delete translateStatus;
    delete translator;
    return 0;
}
