/*
 * CodeBuilderVisitor.h
 *
 *  Created on: Oct 24, 2012
 *      Author: alex
 */

#ifndef CODEBUILDERVISITOR_H_
#define CODEBUILDERVISITOR_H_

#include "ast.h"
#include <stack>

namespace mathvm {

class CodeBuilderVisitor: public mathvm::AstVisitor {
	Code* _code;
	std::stack<Bytecode*> _bytecodes;
public:
	CodeBuilderVisitor(Code* code);
	virtual ~CodeBuilderVisitor();

	void processFunction(AstFunction* top);

#define VISITOR_FUNCTION(type, name) \
    virtual void visit##type(type* node);

    FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION
};

} /* namespace mathvm */
#endif /* CODEBUILDERVISITOR_H_ */
