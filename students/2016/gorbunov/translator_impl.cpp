#include "mathvm.h"

#include <iostream>

#include "pretty_print.h"

using namespace mathvm;

Translator* Translator::create(const string& impl) {
	if (impl == "printer") {
		return new PPrintTranslator(std::cout);
	}
	return nullptr;
}