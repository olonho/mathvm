/*
 * BCTranslator.h
 *
 *  Created on: Dec 8, 2013
 *      Author: sam
 */

#ifndef BCTRANSLATOR_H_
#define BCTRANSLATOR_H_

#include <vector>
#include <iostream>
#include <string>

#include "ast.h"
#include "BCTypes.h"
#include "mathvm.h"
#include "BCInterpreter.h"

namespace mathvm {

class BCTranslator: public AstVisitor {
public:
	BCTranslator(BCInterpreter* inpttr) :
			prog(inpttr), currentVar(0), overflow(false), cType(
					mathvm::VT_INVALID), tType(mathvm::VT_VOID) {
	}
	Status* translate(AstFunction* node);

#define VISITOR_FUNCTION(type, name) virtual void visit##type(type* node);
	FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION

private:
	Bytecode* ip;
	BCInterpreter* prog;
	std::map<std::string, std::vector<VarInt> > vars;
	VarInt currentVar;
	bool overflow;
	VarType cType;
	VarType tType;

	void addVar(AstNode* node, const std::string& name);
	void delVar(const std::string& name);
	void put(const void* vbuf, unsigned int size);
	void putVar(Instruction ins, const AstVar* var, AstNode* node = 0);
	void checkTypeInt(AstNode* expr);
	void triple(Instruction i);

};

} /* namespace mathvm */
#endif /* BCTRANSLATOR_H_ */
