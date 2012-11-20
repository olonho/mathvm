/*
 * main.cpp
 *
 *  Created on: 17.11.2012
 *      Author: Evgeniy Krasko
 */
#include <iostream>
#include <string>
#include "parser.h"
#include "ast_flattener.h"

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

	Parser parser;
	Status * status = parser.parseProgram(source);

	if(status != 0 && status->isError()) {
		cerr << "Error parsing program";
		return 1;
	}

	AstFlattener flattener;
	cout << flattener.flatten(parser.top());
	return 0;
}

