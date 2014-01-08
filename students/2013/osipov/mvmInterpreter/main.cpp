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

using namespace mathvm;
using namespace std;

int main(int argc, char** argv) {
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << "/path/to/program.mvm" << endl;
        return 1;
    }

    string program(loadFile(argv[1]));

    Translator* translator = new MvmTranslator();

    cout << "------ Program ------" << endl;
    cout << program << endl;
    cout << "---------------------" << endl;
    
    Code* code;
    if (Status * s = translator -> translate(program, &code)) {
        cerr << "Translate Error: " << s -> getError() << " at" << s -> getPosition() << endl;
        delete s;
        delete code;
        delete translator;
        return 1;
    }

    delete translator;
    delete code;
    return 0;
}


