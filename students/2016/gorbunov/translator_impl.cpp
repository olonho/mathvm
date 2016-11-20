#include "mathvm.h"

#include "translate_to_bytecode.h"
#include "pretty_print.h"

using namespace mathvm;

Translator* Translator::create(const string& impl) {
	if (impl == "printer") {
		return new PPrintTranslator(std::cout);
	} else if (impl == "translator") {
		return new BytecodeGenTranslator();
	}
	return nullptr;
}