/*
 * AstToCodePrinter.h
 *
 *  Created on: Oct 10, 2013
 *      Author: Semen Martynov
 */

#ifndef ASTTOCODEPRINTER_H_
#define ASTTOCODEPRINTER_H_

#include "ast.h"

namespace mathvm
{

class AstToCodePrinter: public AstVisitor
{
public:
	AstToCodePrinter(ostream& somestream = std::cout): _stream(somestream) {}
	virtual ~AstToCodePrinter() {}

	void exec(AstFunction * top);
	void processBlockNode(BlockNode * node);

#define VISITOR_FUNCTION(type, name) virtual void visit##type(type* node);
	FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION
private:
	ostream& _stream;
};

} /* namespace mathvm */
#endif /* ASTTOCODEPRINTER_H_ */
