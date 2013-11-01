#ifndef TRANSLATOR_VISITOR_H_
#define TRANSLATOR_VISITOR_H_
#include <mathvm.h>
#include <ast.h>
#include <visitors.h>
#include <parser.h>

#include "CodeImpl.h"

namespace mathvm {

struct Error {
	Error(std::string msg, uint32_t pos) : msg(msg), pos(pos) {}

	std::string msg;
	uint32_t pos;
};

struct ScopeVar {
	ScopeVar(uint16_t scope_id, ScopeVar *p):
		parent(p), scope_id(scope_id){}

	void addVar(AstVar *v) {
		vars.insert(std::make_pair(v, vars.size()));
	}

	std::pair<uint16_t, uint16_t> getVar(const AstVar *v) {
		std::map<const AstVar*, uint16_t>::iterator iter = vars.find(v);
		if (iter != vars.end())
			return std::make_pair(iter->second, scope_id);
		else {
			if (!parent)
				throw std::string("Var " + v->name() + " not found");;
			return parent->getVar(v);
		}
	}

	ScopeVar * parent;
	uint16_t scope_id;
	std::map<const AstVar*, uint16_t> vars;
};


struct TranslatorVisitor: AstVisitor{
	Status * translate(const string &program, CodeImpl ** code);

#define VISITOR_FUNCTION(type, name) \
		virtual void visit##type(type* node);

	FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION

private:
	void processFunction(AstFunction * f);
	void convertToBool();
	void loadVar(const AstVar * var);
	void storeVar(const AstVar * var);

	CodeImpl * code;
	ScopeVar * currentScope;
	BytecodeFunction * currentFunction;
	VarType lastExpressionType;
};
}
#endif

