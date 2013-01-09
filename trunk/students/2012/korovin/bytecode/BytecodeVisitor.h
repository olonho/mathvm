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
#include <vector>
#include <AsmJit/Assembler.h>
#include <AsmJit/Logger.h>
#include <dlfcn.h>

namespace mathvm {

using namespace AsmJit;

class BytecodeVisitor : public AstVisitor {
	AstFunction* top_;
	Code* code_;
    uint16_t varId;
    VarType typeOfTOS;

	stack<BytecodeFunction*> functionsStack_;
	stack<Scope*> scopesStack_;
	map<AstFunction*, BytecodeFunction*> functions_;
	map<const AstVar*, pair<uint16_t, uint16_t> > vars_;

	vector<XMMReg> xmmRegisters_;
	vector<GPReg> gpRegisters_;

	void functionDeclarations( Scope* scope );
	void variableDeclarations( Scope* scope );
	void blockContents(BlockNode* node);
	Bytecode* bc() {
		return functionsStack_.top()->bytecode();
	}
	void compare(Instruction kind);
public:
	BytecodeVisitor(AstFunction* top, Code* code);
	void visit();
	virtual ~BytecodeVisitor();
    void store(uint16_t functionId, uint16_t id, VarType type);

#define VISITOR_FUNCTION(type, name)            \
    virtual void visit##type(type* node);

    FOR_NODES(VISITOR_FUNCTION)
       
    void toBoolean();

#undef VISITOR_FUNCTION
};

} /* namespace mathvm */
#endif /* BYTECODEVISITOR_H_ */
