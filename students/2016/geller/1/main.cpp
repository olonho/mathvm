//
// Created by Mark Geller on 13.10.16.
//
#include "../../../../include/ast.h"
#include "../../../../vm/parser.h"
#include "translator_impl.h"



using namespace mathvm;

int main(int argc, char* argv[]){
    if(argc != 2){
        cout << "invalid number of arguments, need exactly one: filename";
        return 0;
    }
    char* code = loadFile(argv[1]);
    if(code == nullptr){
        cout << "failed to retrieve file contents";
        return 0;
    }
    Parser parser;
    Status* status = parser.parseProgram(code);
    if(status->isError()){
        cout << "Can not parse provided file. check program syntax";
        return 0;
    }
    ASTPrinter t(parser.top(), std::cout);
    bool pstatus = t.print();
    cout << "\n\n\n\n";
    if(pstatus){
        cout << "Print successful";
    }else{
        cout << "print failed ";
    }
}
