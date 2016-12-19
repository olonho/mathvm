#include "interpreter_impl.h"

using namespace mathvm;

int main(int argc, char** argv) {
    if (argc <= 1) {
        cout << "No source file parameter found" << endl;
        return 1;
    }
    const char* fileName = argv[1];
    const char* sourceCode = loadFile(fileName);
    if (sourceCode == 0) {
        cout << "Cannot read file: " << fileName << endl;
        return 1;
    }

    Code* code = new InterpreterCodeImpl();
    Translator* translator = Translator::create();
    Status* translateStatus = translator->translate(sourceCode, &code);
    if (translateStatus->isError()) {
        uint32_t position = translateStatus->getPosition();
        uint32_t line = 0, offset = 0;
        positionToLineOffset(sourceCode, position, line, offset);
        printf("Cannot translate expression: expression at %d,%d; "
                   "error '%s'\n", line, offset, translateStatus->getError().c_str());
    } else {
//        code->disassemble();
        vector<Var*> emptyVars;
        Status* executeStatus = code->execute(emptyVars);
        if (executeStatus->isError()) {
            uint32_t position = executeStatus->getPosition();
            uint32_t line = 0, offset = 0;
            positionToLineOffset(sourceCode, position, line, offset);
            printf("Interpreter error: %s", executeStatus->getError().c_str());
        }
        delete executeStatus;
    }
    delete translateStatus;
    delete translator;
    delete code;
    delete[] sourceCode;
    return 0;
}

