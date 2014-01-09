#include <cstdlib>
#include <iostream>
#include <fstream>
#include <iterator>
#include <ctime>

#include <parser.h>
#include <ast.h>

#include "CompilerVisitor.h"
#include "StackVisitor.h"
#include "Interpreter.h"

/*
my vm have fake registers:
op1, op2 - for binary, unary operations, if expression
eax - for return value
ebp - current frame base pointer
esp - current top of stack

IVAR0 - eax
DVAR0 - eax
SVAR0 - eax

IVAR1 - ebp
IVAR2 - esp
*/

using namespace mathvm;

int main(int argc, char* argv[])
{
	if (argc != 2)
	{
		std::cout << "Usage: " << argv[0] << " <input source>" << std::endl;
		exit(1);
	}

	std::string   source_code;
	std::ifstream input(argv[1]);
	input.unsetf(std::ios_base::skipws);
	std::copy(std::istream_iterator<char>(input), std::istream_iterator<char>(), std::back_inserter(source_code));

	Parser parser;
	if (Status* s = parser.parseProgram(source_code))
	{
		std::cerr << "PARSE ERROR: " << s->getError() << std::endl;
		exit(1);
	}

	AstFunction* f = parser.top();

	StackVisitor sv;
	sv.callStartFunction(f);

	CompilerVisitor compiler;
	compiler.visitStartFunction(f, sv.result());

	const vector<Bytecode>& functions = compiler.bytecodes();
	//for (size_t i = 0; i < functions.size(); ++i)
	//{
	//	cout << "function " << i << endl;
	//	functions[i].dump(std::cout);
	//}

	Interpreter i;
	i.execute(functions, compiler.literals());

	return 0;
}
