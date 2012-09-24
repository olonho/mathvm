/*
 * main.cpp
 *
 *  Created on: Sep 23, 2012
 *      Author: Alexander Opeykin (alexander.opeykin@gmail.com)
 */

#include <fstream>
#include <iterator>

#include "parser.h"
#include "ast_printer.h"

using namespace mathvm;

int main(int argc, char** argv) {
    if (argc == 1) {
        cout << "enter path to source code as parameter" << endl;
        return 1;
    }

    ifstream ifs (argv[1]);
    if (!ifs) {
        cout << "can not open file: " << argv[1] << endl;
        return 1;
    }

    string code(
            (istreambuf_iterator<char>(ifs)),
            istreambuf_iterator<char>()
    );

    Parser parser;
    Status* status = parser.parseProgram(code);

    if (status) {
        if (status->isError()) {
            cout << status->getError() << endl;
            delete status;
            return 1;
        }
    }

    AstFunction* top = parser.top();
    AstPrinter printer(cout);

    top->node()->body()->visit(&printer);

    delete status;

    return 0;
}




