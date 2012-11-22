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
#include "CodeBuilderVisitor.h"
#include "interpreter_code_impl.h"

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

    Code* code = new InterpreterCodeImpl(cout);
    CodeBuilderVisitor visitor(code);
    visitor.start(parser.top());

   	if (argc > 2) {
   		code->disassemble();
   	}

   	if (argc == 3) {
   		cout << endl << "---------------------" << endl;
   	}

   	if (argc == 4) {
   		return 0;
   	}

	vector<Var*> vars;
	Status* exec_status = code->execute(vars);

	if (exec_status) {
		cout << exec_status->getError() << endl;
		delete exec_status;
		return 1;
	} else {
		cout << endl;
	}

	delete exec_status;
    delete status;

    return 0;
}




