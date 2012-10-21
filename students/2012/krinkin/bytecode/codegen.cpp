#include "bytecodeimpl.h"
#include "codegen.h"
#include "parser.h"

#include <memory>

Status* CodeGenerator::translate(const string& program, Code* *code)
{
	std::auto_ptr<Status> status;
	
	Parser parser;
	status.reset(parser.parseProgram(program));
	if (status.get())
	{
		Status *ret = status.get();
		status.release();
		return ret;
	}
	
	TypeChecker checker;
	status = checker.check(parser.top());
	if (status.get())
	{
		Status *ret = status.get();
		status.release();
		return ret;
	}
	
	std::auto_ptr<BytecodeImpl> translated(new BytecodeImpl());
	AstTranslator translator;
	status = translator.translate(parser.top(), translated.get());
	if (status.get())
	{
		Status *ret = status.get();
		status.release();
		return ret;
	}
	
	*code = translated.get();
	translated.release();
	
	return 0;
}
