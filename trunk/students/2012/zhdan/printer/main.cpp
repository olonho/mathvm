/*
 * main.cpp
 *
 *  Created on: Oct 23, 2012
 *      Author: user
 */
#include "printer.h"
#include "parser.h"
#include <iostream>

using namespace mathvm;
using namespace std;

int main(int argsCount, char **args) {
	if (argsCount < 2) {
		cout << "Alert!";
		return 0;
	}

	char* content = loadFile(args[1]);
	if (content == NULL) {
		cout << "Error loading file";
		return 0;
	}
	Parser parser;
	Status* status = parser.parseProgram(content);
	if ((status != NULL) && (status->isError())) {
		cout <<"Bad status";
		return 0;
	}
	delete status;

	PrinterVisitor printer(cout);
	printer.printAst(parser.top());
	return 0;
}



