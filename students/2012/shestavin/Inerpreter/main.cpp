//
//  main.cpp
//
//  Created by Dmitriy on 9/24/12.
//  Copyright (c) 2012 Dmitriy. All rights reserved.
//

#include <iostream>
#include "parser.h"
#include "mathvm.h"
#include "bytecode_visitor.h"
#include "interprer.h"


using namespace mathvm;
using namespace std;

int main (int argc, char** argv) {
    if (argc <= 1) {
        cout << "Path to source expected." << endl;
        return 1;
    } else if (argc > 2) {
        cout << "Too many arguments." << endl;
        return 2;
    }
    
    char* chbuf = loadFile(argv[1]);
    if (!chbuf) {
        cout << "Can't read file with name :'" << argv[1] << "'" << endl;
        delete[] chbuf;
        return 3;
    }
    
    const string codeStr(chbuf);
    Parser parser;
    Status* status = parser.parseProgram(codeStr);
    if (status && status->isError()) {
        cout << status->getError() << endl;
        delete[] chbuf;
        delete status;
        return 4;
    }
    Code *code = new Interpreter();
    BytecodeVisitor* bv = new BytecodeVisitor(parser.top(), code);
    bv->visit();
    //code->disassemble();
    //cout << endl << "Executing:" << endl;
    vector<Var*> v;
    code->execute(v);
    
    delete[] chbuf;
    delete status;
    return 0;
}