#pragma once

#include <iostream>
#include "ast.h"

class AstPrinter : public mathvm::AstVisitor {
public:
	AstPrinter(std::ostream& out);
	virtual ~AstPrinter() {}

	void printBlockContent(mathvm::BlockNode* node);

#define VISITOR_FUNCTION(type, name) \
	virtual void visit##type(mathvm::type* node);

	FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION

private:
	void printIndent();
	void printVarType(mathvm::VarType type);

	int getPriority(mathvm::TokenKind);

	void escape(std::string& s);
	void replaceAll(std::string& s, const std::string& x, const std::string& y);

	int indent;
	std::ostream& out;
};
