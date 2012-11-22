#include "mathvm.h"
#include "parser.h"
#include "BytecodeGenerationVisitor.h"
#include "InterpreterCodeImpl.h"


Status* BytecodeTranslatorImpl::translateBytecode(const string& program, InterpreterCodeImpl* *code) {
	Parser parser;
    Status *pStatus = parser.parseProgram(program);
    if(pStatus && (pStatus -> isError())) {
    	return new Status(pStatus -> getError());
    }

    *code = new InterpreterCodeImpl();

    try {
    	parser.top() -> visit(new BytecodeGenerationVisitor(*code));	
    } catch (std::exception e) {
    	return new Status(e.what());
    }
    
    return new Status();
}

Status* BytecodeTranslatorImpl::translate(const string& program, Code* *code) {
	return translateBytecode(program, (InterpreterCodeImpl**)code);
}