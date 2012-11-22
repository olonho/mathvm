/*
 * BytecodeVisitor.h
 *
 *  Created on: 24.10.2012
 *      Author: temp1ar
 */

#ifndef BYTECODEVISITOR_H_
#define BYTECODEVISITOR_H_

#include "ast.h"
#include <stack>
#include <map>

namespace mathvm {

class BytecodeVisitor : public AstVisitor {
	AstFunction* top_;
	Code* code_;
    uint16_t varId;
    VarType typeOfTOS;

	stack<BytecodeFunction*> functionsStack_;
	stack<Scope*> scopesStack_;
	map<AstFunction*, BytecodeFunction*> functions_;
	map<const AstVar*, pair<uint16_t, uint16_t>> vars_;

	void functionDeclarations( Scope* scope );
	void variableDeclarations( Scope* scope );
	void blockContents(BlockNode* node);
	Bytecode* bc() {
		return functionsStack_.top()->bytecode();
	}
	void compare();
public:
	BytecodeVisitor(AstFunction* top, Code* code);
	void visit();
	virtual ~BytecodeVisitor();
    void store(uint16_t functionId, uint16_t id, VarType type);

#define VISITOR_FUNCTION(type, name)            \
    virtual void visit##type(type* node);

    FOR_NODES(VISITOR_FUNCTION)

#undef VISITOR_FUNCTION
};

} /* namespace mathvm */
#endif /* BYTECODEVISITOR_H_ */
