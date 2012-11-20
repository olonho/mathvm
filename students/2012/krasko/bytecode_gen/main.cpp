/*
 * main.cpp
 *
 *  Created on: 17.11.2012
 *      Author: Evgeniy Krasko
 */
#include <iostream>
#include <string>
#include "parser.h"
#include "bytecode_gen.h"
#include "bc_interpreter.h"

using namespace std;
using namespace mathvm;

int main(int argc, char **argv) {
	if (argc < 2) {
		cerr << "Enter source file name." << endl;
		return 1;
	}

	const char * source = loadFile(argv[1]);
	if (source == 0) {
		cerr << "Error loading file.";
		return 1;
	}

	BC_Interpreter code;

	BytecodeGen generator;
	generator.generateCode(&code, source);

	code.disassemble(std::cout);

	cout << endl;

	vector<Var*> v;
	code.execute(v);
	return 0;
}

