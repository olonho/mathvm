//
//  main.cpp
//
//  Created by Dmitriy on 9/24/12.
//  Copyright (c) 2012 Dmitriy. All rights reserved.
//

#include <iostream>
#include "parser.h"
#include "ast_analyzer.h"


using std::cout;
using std::endl;
using std::string;

using mathvm::loadFile;
using mathvm::Parser;
using mathvm::Status;

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
    
    const string code(chbuf);
    Parser parser;
    Status* status = parser.parseProgram(code);
    if (status && status->isError()) {
        cout << status->getError() << endl;
        delete[] chbuf;
        delete status;
        return 4;
    }
    ASTAnalyzer analyzer(cout);
    parser.top()->node()->body()->visit(&analyzer);
    
    delete[] chbuf;
    delete status;
    return 0;
}