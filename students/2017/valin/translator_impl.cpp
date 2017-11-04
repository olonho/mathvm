#include "mathvm.h"
#include "pretty_printer.h"
#include "bytecode_translator.h"

#include <string>
#include <iostream>

using mathvm::Translator;

Translator* Translator::create(const string& impl)
{
	if (impl == "printer") {
		return new my::AstPrinter();
	} else if (impl == "interpreter" || impl == "") {
		return new my::BytecodeTranslator();
	}

	return nullptr;
}
