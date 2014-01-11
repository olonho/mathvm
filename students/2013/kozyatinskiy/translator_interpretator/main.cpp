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
#include "MyCompiler.h"

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

	try
	{
		AstFunction* f = parser.top();

		StackVisitor sv;
		sv.callStartFunction(f);

		CompilerVisitor compiler;

		compiler.visitStartFunction(f, sv.result());

		const vector<pair<VarType, Bytecode_> >& functions = compiler.bytecodes();
		//for (size_t i = 0; i < functions.size(); ++i)
		//{
		//	cout << "function " << i << endl;
		//	functions[i].second.dump(std::cout);
		//}

		Interpreter interp;
		MyCompiler jit;
		interp.setCompiler(&jit);
		interp.execute(functions, compiler.literals());

		//for(size_t i = 0; i < interp.callsCount().size(); ++i)
		//{
		//	cout << i << " : " << interp.callsCount()[i] << endl;
		//}
	}
	catch(std::exception& e)
	{
		cout << "fatal error: " << e.what() << endl;
	}
	return 0;
}
