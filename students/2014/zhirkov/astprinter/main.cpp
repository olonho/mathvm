#include "../../../../include/mathvm.h"

#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <iostream>

using namespace mathvm;
using namespace std;

const char* programTextOrFailure(int argc, char** argv) {
    if ( argc < 2 || argv[1] == NULL ) {
        std::cerr << "First argument should be script name!" << std::endl;
        exit(EXIT_FAILURE);
    }
    const char* filename = argv[1];
    const char* text = loadFile( filename );
    if ( text == NULL ) {
        std::cerr << "Can't read file: " << filename << std::endl;
        exit(EXIT_FAILURE);
    }
    return text;
}

int main( int argc, char** argv ) {

    Translator* translator = Translator::create("printer");
    const char* program = programTextOrFailure( argc, argv );
    Code* code = NULL;
    Status* translateStatus = translator->translate( program, &code );
    if ( translateStatus->isError() )
    {
        uint32_t position = translateStatus-> getPosition();
        uint32_t line = 0, offset = 0;
        positionToLineOffset( program, position, line, offset );
        std::cerr << "Can't translate expression: expression at " << line << "," << offset << std::endl << "error: '" <<
               translateStatus-> getError() << "'" << std::endl;
    } 
    delete translateStatus;
    delete translator;

    delete[] program;

    return EXIT_SUCCESS;
}
