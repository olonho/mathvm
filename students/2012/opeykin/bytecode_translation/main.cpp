/*
 * main.cpp
 *
 *  Created on: Oct 23, 2012
 *      Author: alex
 */

#include <fstream>
#include <iterator>

#include "parser.h"
#include "mathvm.h"
#include "ExecutableCode.h"
#include "CodeBuilderVisitor.h"

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

    string source(
            (istreambuf_iterator<char>(ifs)),
            istreambuf_iterator<char>()
    );

    Parser parser;
    Status* status = parser.parseProgram(source);

    if (status) {
        if (status->isError()) {
            cout << status->getError() << endl;
            delete status;
            return 1;
        }
    }

    Code* code = new ExecutableCode;
    CodeBuilderVisitor visitor(code);
    visitor.processFunction(parser.top());
    code->disassemble();
//    BytecodeFunction bf(top);
//    bf.disassemble(cout);
//    AstPrinter printer(cout);
//
//    top->node()->body()->visit(&printer);

    delete status;

    return 0;
}




