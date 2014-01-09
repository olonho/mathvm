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
#include "mathvm.h"
#include "BCInterpritator.h"

namespace mathvm {

class BCTranslator: public AstVisitor {
public:
	BCTranslator(Code* inpttr) :
			prog(inpttr), currentVar(0), overflow(false), cType(
					mathvm::VT_INVALID), tType(mathvm::VT_VOID) {
	}
	Status* translate(AstFunction* node);

#define VISITOR_FUNCTION(type, name) virtual void visit##type(type* node);
	FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION

private:
	Bytecode* ip;
	Code* prog;
	typedef uint16_t VarInt;
	std::map<std::string, std::vector<VarInt> > vars;
	VarInt currentVar;
	bool overflow;
	VarType cType;
	VarType tType;

	VarInt addVar(AstNode* node, const std::string& name);
	void delVar(const std::string& name);
	void put(const void* vbuf, unsigned int size);
	void checkTypeInt(AstNode* expr);
	void triple(Instruction i);

	template<class T>
	void putVar(Instruction ins, T* node) {
		ip->add(ins);
		const std::vector<VarInt>& v = vars[node->var()->name()];
		if (v.empty()) {
			std::cerr << "Error: undeclared variable: "
					<< node->var()->name().c_str() << std::endl;
		}
		put(&v.back(), sizeof(VarInt));
	}
};

} /* namespace mathvm */
#endif /* BCTRANSLATOR_H_ */
