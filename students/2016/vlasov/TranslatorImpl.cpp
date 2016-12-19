//
// Created by svloyso on 17.11.16.
//

#include "mathvm.h"
#include "parser.h"
#include "translator_visitor.h"

namespace mathvm {

Translator* Translator::create(const string& impl) {
    if (impl == "" || impl == "intepreter") {
        return new BytecodeTranslatorImpl();
    }
    if (impl == "jit") {
        return new MachCodeTranslatorImpl();
    }
    assert(false);
    return 0;
}

Status* BytecodeTranslatorImpl::translate(const std::string& program, Code** code) {
	Parser parser;
	Status* status = parser.parseProgram(program);
	if(!status->isOk()) {
		return status;
	}

	TranslatorVisitor vis(*code);
	try {
		vis.declareScope(parser.top()->scope(), true);
	} catch (Status* s) {
		return s;
	}

	return Status::Ok();
}

Status* BytecodeTranslatorImpl::translateBytecode(const std::string& program, InterpreterCodeImpl* *code) {
	return nullptr;
}

} // namespace mathvm
