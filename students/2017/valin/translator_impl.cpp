#include "mathvm.h"
#include "parser.h"
#include "pretty_printer.h"

#include <string>
#include <iostream>

using mathvm::Translator;
using mathvm::Status;

class AstPrinterTranslator : public Translator
{
	virtual Status * translate(const std::string& source, mathvm::Code ** program) override
	{	
		mathvm::Parser parser;
		Status * status = parser.parseProgram(source);
		my::AstPrinter::pretty_printer(parser.top(), std::cout);
		return status;
	}
};

Translator* Translator::create(const string& impl)
{
	if (impl == "printer") {
		return new AstPrinterTranslator();
	}

	return nullptr;
}
