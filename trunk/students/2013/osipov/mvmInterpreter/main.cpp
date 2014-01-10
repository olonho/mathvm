/* 
 * File:   main.cpp
 * Author: stasstels
 *
 * Created on January 3, 2014, 3:56 PM
 */
#include <cstdlib>
#include <iostream>
#include <fstream>

#include "MvmTranslator.h"
#include "MvmInterpreter.h"

using namespace mathvm;
using namespace std;

int main(int argc, char** argv) {
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << "   /path/to/program.mvm  /path/to/expected" << endl;
        return 1;
    }

    string program(loadFile(argv[1]));
    string expected(loadFile(argv[2]));

    Translator* translator = new MvmTranslator();

//    cout << "------ Program ------" << endl;
//    cout << program << endl;
//    cout << "---------------------" << endl;
//
//    cout << "------ Expected ------" << endl;
//    cout << expected << endl;
//    cout << "---------------------" << endl;


    Code* code;
    if (Status * s = translator -> translate(program, &code)) {
        cerr << "Translate Error: " << s -> getError() << endl;
        delete s;
        delete code;
        delete translator;
        return 1;
    }

//    cout << "------ Bytecode ------" << endl;
//    code -> disassemble(cout);
//    cout << "---------------------" << endl;


//    cout << "------ Interpretation ------" << endl;
    vector<Var*> env;
    if (Status * s = code -> execute(env)) {
        std::cerr << "Interpret error: " << s -> getError() << endl;
        delete s;
        delete code;
        delete translator;
        return 1;
    }
//   cout << "---------------------" << endl;
    delete translator;
    delete code;
    return 0;
}


