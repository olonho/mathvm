#include "bytecoder.h"
#include "my_translator.h"
#include "mathvm.h"
#include "parser.h"
#include <iostream>
#include <iomanip>

namespace mathvm {

Translator* Translator::create(const string& impl) {
	if (impl == "my_translator") {
		return new MyTranslator();
	}
	assert(false);
	return 0;
}

Status* MyTranslator::translate(const string& program, Code* *code) {
	Parser* parser = new Parser();
	Status* status = parser->parseProgram(program);
	if (status == NULL) {
		BytecodeFunction* pseudo_function = new BytecodeFunction(parser->top());
		(*code)->addFunction(pseudo_function);
		Bytecoder* visitor = new Bytecoder(*code);
		parser->top()->node()->visit(visitor);
		pseudo_function->bytecode()->add(BC_STOP);
		delete visitor;
	}
	delete parser;
	return status;
}

}
