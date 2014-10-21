#include "visitors.h"
#include "mathvm.h"
#include <iostream>

#ifndef PRINT_VISITOR_H
#define PRINT_VISITOR_H
using namespace mathvm;

class PrintVisitor: public AstVisitor {
public:
	std::ostream& out;
	PrintVisitor(std::ostream& _out, int _indentSize);
	PrintVisitor(std::ostream& _out);
	#define VISITOR_FUNCTION(type, name)            \
	    virtual void visit##type(type* node);
	
	    FOR_NODES(VISITOR_FUNCTION)
	#undef VISITOR_FUNCTION
private:
	int indentSize;
	int indent;
	bool needSemicolon;
	void printIndent();
};
#endif