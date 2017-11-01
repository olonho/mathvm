#include "mathvm.h"
#include "pretty_printer.h"

#include <string>
#include <iostream>

using mathvm::Translator;

Translator* Translator::create(const string& impl)
{
	if (impl == "printer") {
		return new my::AstPrinter();
	}

	return nullptr;
}
