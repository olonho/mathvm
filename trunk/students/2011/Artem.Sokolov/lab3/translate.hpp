#ifndef _TRANSLATE_HPP_
#define _TRANSLATE_HPP_

#include "mathvm.h"
#include "visitor.hpp"

class MyTranslator: public mathvm::Translator {
public:
    mathvm::Status* translate(const std::string& program, mathvm::Code* *code) {
    	mathvm::Parser* parser = new mathvm::Parser();
    	mathvm::Status* status = parser->parseProgram(program);
		if (status == NULL) {
			mathvm::BytecodeFunction* top_function = new mathvm::BytecodeFunction(parser->top());
			(*code)->addFunction(top_function);
			Visitor* visitor = new Visitor(*code);
			parser->top()->node()->visit(visitor);
			top_function->bytecode()->add(mathvm::BC_STOP);
			delete visitor;
		}
		delete parser;
		return status;
    }
};

mathvm::Translator* mathvm::Translator::create(const string& impl) {
	return new MyTranslator();
}

#endif // _TRANSLATE_HPP_
