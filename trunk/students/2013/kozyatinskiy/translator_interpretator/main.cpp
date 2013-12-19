#include <cstdlib>
#include <iostream>
#include <fstream>
#include <iterator>

#include "vm/parser.h"
#include "vm/ast.h"

#include "PrintVisitor.h"
#include "MyInterpreter.h"

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

//	std::cout << source_code << std::endl;

	Parser parser;
	if (Status* s = parser.parseProgram(source_code))
	{
		std::cerr << "PARSE ERROR: " << s->getError() << std::endl;
		exit(1);
	}

	AstFunction* top = parser.top();

	MyProgram p;
	PrintVisitor node_printer(std::cout, &p);
	try
	{
		node_printer.process(top);
		p.dump(cout);
		p.removeAllLabels();
		cout << "--------" << endl;
		p.dump(cout);
		MyBytecode* bc = p.link();
		cout << "--------" << endl;
		bc->dump(cout);

		cout << "------" << endl;
		MyInterpreter interpreter(bc);
		interpreter.run();
	}
	catch(const std::logic_error& e)
	{
		std::cout << e.what() << std::endl;
	}
	return 0;
}