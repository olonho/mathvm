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
#include <map>

namespace mathvm {

class CodeBuilderVisitor: public mathvm::AstVisitor {
	typedef std::map<string, uint16_t> VarScopeMap;

	Code* _code;
	std::stack<Bytecode*> _bytecodes;
	std::stack<VarScopeMap> _variables;
public:
	CodeBuilderVisitor(Code* code);
	virtual ~CodeBuilderVisitor();

	void processFunction(AstFunction* top);

#define VISITOR_FUNCTION(type, name) \
    virtual void visit##type(type* node);

    FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION

private:
	Bytecode* curBytecode() {
		return _bytecodes.top();
	}

	VarScopeMap& curVarMap() {
		return _variables.top();
	}

	uint16_t getId(const AstVar* var) {
		return curVarMap()[var->name()];
	}
};

} /* namespace mathvm */
#endif /* CODEBUILDERVISITOR_H_ */
