#pragma once

#include <iostream>
#include "ast.h"

class AstPrinter : public mathvm::AstVisitor {
public:
	AstPrinter(std::ostream& out);
	virtual ~AstPrinter() {}

	void printAst(mathvm::AstFunction* topfn);

#define VISITOR_FUNCTION(type, name) \
	virtual void visit##type(mathvm::type* node);

	FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION

private:
	void printIndent();
	void printVarType(mathvm::VarType type);
	void printVarDecl(mathvm::AstVar* var);
	void printFunc(mathvm::AstFunction* fn);

	int indent;
	std::ostream& out;
};
