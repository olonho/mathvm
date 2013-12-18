#pragma once

#include "ast.h"
#include "InterpreterCodeImpl.h"

class BcTranslator : public mathvm::Translator, private mathvm::AstVisitor
{
public:
	BcTranslator();
	virtual ~BcTranslator() {}

	// inherited from Translator
	virtual mathvm::Status* translate(const std::string& program, mathvm::Code** code);

private:
	// inherited from Visitor
#define VISITOR_FUNCTION(type, name) \
	virtual void visit##type(mathvm::type* node);

	FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION

	InterpreterCodeImpl* resultCode;
};
