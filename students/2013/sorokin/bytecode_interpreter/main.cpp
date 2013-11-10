#include <fstream>
#include <iterator>

#include "CodeTranslator.h"
#include "TypeCalculator.h"

#include "parser.h"
#include "mathvm.h"
#include "InterpreterCodeImpl.h"

using namespace mathvm;

string get_source_code(char const * filename) {
 
    char *buffer =  mathvm::loadFile(filename);
    return string(buffer);
}


int main(int argc, char** argv) {
    if(argc != 3) {
        cerr << "wrong number of arguments." 
             << "please input path to source file and flag of excution type" << endl;
        return 1;
    }
    
    char flag = argv[2][0]; 
    
    if(flag != 't' && flag != 'i') {
        cerr << "invalid target type." << endl 
             << "\'i\' - for translation and interperetation " << endl
             << "\'t\' - for translation only" << endl;
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
    
    Code* code = new InterpreterCodeImpl(cout);
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

    if(flag == 'i' && translationIsOK)  {
        vector<Var*> vars;
        cout << "-----execution---------------" << endl;
        Status* exec_status = code->execute(vars);
        cout << "-----/execution---------------" << endl;

        if (exec_status) {
            cout << "exec_status error:" <<  exec_status->getError() << endl;
            delete exec_status;
            
        } else {    
            cout << "-----/all correct!---------------" << endl;
        }
        delete exec_status;
    }
    
    delete code;
    delete types;
    
    return !translationIsOK;
}



