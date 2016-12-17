#include <mathvm.h>

#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <iostream>

#include "code_interpreter.h"
#include "code_generator.h"
#include "translator_impl.h"

using namespace mathvm;
using namespace std;

int main(int argc, char** argv) {
    if (argc < 2) 
    {
        cout << "Input source file as a parameter!" << endl;
        return 1;
    }
    const char* fileName = argv[1];
    const char* inputCode = loadFile(fileName);

    Code* code = new CodeInterpreter();
    Code** argument = &code;
    Translator* translator = Translator::create();
    Status* translateStatus = translator->translate(inputCode, argument);
    
    if (translateStatus->isError()) 
    {
        uint32_t position = translateStatus->getPosition();
        uint32_t line = 0, offset = 0;
        positionToLineOffset(inputCode, position, line, offset);
        printf("Cannot translate expression: expression at %d,%d; "
               "error '%s'\n", 
               line, 
               offset,
               translateStatus->getErrorCstr());
    } 
    else 
    {
        vector<Var*> vars;
        Status* execStatus = code->execute(vars);
        if (execStatus->isError()) 
        {
            printf("Interpreter error: %s", execStatus->getError().c_str());
        }
        
        delete execStatus;
    }
    
    delete translateStatus;
    delete translator;    
    delete code;
    delete[] inputCode;

    return 0;
}
