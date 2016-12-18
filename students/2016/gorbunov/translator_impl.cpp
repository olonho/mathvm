#include "mathvm.h"

#include "pretty_print.h"
#include "bytecode_interpreter.h"

using namespace mathvm;

Translator* Translator::create(const string& impl) {
	if (impl == "printer") {
		return new PPrintTranslator(std::cout);
	} else if (impl == "translator") {
		return new BytecodeGenTranslator();
	} else if (impl == "interpreter") {
		return new BytecodeInterpreterTranslator(std::cout);
	}
	return nullptr;
}