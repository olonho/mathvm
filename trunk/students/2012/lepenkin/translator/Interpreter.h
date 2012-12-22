/*
 * Interpreter.h
 *
 *  Created on: Dec 17, 2012
 *      Author: yarik
 */

#ifndef INTERPRETER_H_
#define INTERPRETER_H_

#include <mathvm.h>
#include <ast.h>
#include <iostream>
#include <string>
#include <stack>


using std::string;
using std::cin;
using std::cout;
using std::ostream;
#include <stdlib.h>

using namespace mathvm;
using std::stack;


union StackVar {
	uint16_t strId;
	int i;
	double d;
};


class Interpreter {



public:
	Interpreter(ostream& out);
	virtual ~Interpreter();

    void execute(Code* code);
    void setLogger(ostream* out);


private:
	ostream& _out;
	ostream* _log;


	Bytecode* _bc;
	uint32_t _insPtr;


	stack<StackVar> _stack;
	stack<VarType> _typeStack;

	void loadDouble();
	void loadString();
	void loadInt();



};

#endif /* INTERPRETER_H_ */
