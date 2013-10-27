#include <fstream>
#include <iterator>

#include "mathvm.h"
#include "parser.h"

#include "CodeTranslator.h"
#include "TypeCalculator.h"
#include "MockInterpreter.h"

using namespace mathvm;

string get_source_code(char const * filename) {
 
    char *buffer =  mathvm::loadFile(filename);
    return string(buffer);
}


int main(int argc, char** argv) {
    if(argc != 2) {
        cerr << "wrong number of arguments." 
             << "please input path to source file" << endl;
        return 1;
    }
    
    string source = get_source_code(argv[1]);
    
    
    Parser parser;
    Status *pStatus = parser.parseProgram(source);
    if(pStatus && pStatus->isError()) {
        cerr << pStatus->getError();
        return 1;
    }
    
    AstTypeMap *types = new AstTypeMap(); // nodes and Vars to -> VarTypes
    TypeCalculator typer(*types, cerr); 
    typer.calculateTypes(parser);
    
    Code* code = new MockInterpreter();
    CodeTranslator translator(code);
    
    bool translationIsOK = true;

    try {
        translator.translate(parser.top(), types);
    } catch(ExceptionInfo e) {
        cerr << e.what() << endl;
        translationIsOK = false;
    }
    
    cout << "-----bytecode---------------" << endl;
    code->disassemble(cout);
    cout << "----/bytecode---------------" << endl;

    delete code;
    delete types;
    
    return !translationIsOK;
}



