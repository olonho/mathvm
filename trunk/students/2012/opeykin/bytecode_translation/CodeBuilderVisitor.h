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

struct VarInfo {
	VarInfo(const uint16_t id, const uint16_t functionId)
			: id(id), context(functionId) { }
	const uint16_t id;
	const uint16_t context;
};

class VarScopeMap {
public:
	typedef const AstVar* Key;

	VarScopeMap(uint16_t context, VarScopeMap* parent)
			: _context(context), _parent(parent) {
		if (_parent != 0) {
			_nextId = parent->nextId();
		} else {
			_nextId = 0;
		}
	}

	bool contains(Key key) {
		return _map.find(key) != _map.end();
	}

	VarInfo get(Key key) {
		if (contains(key)) {
			return VarInfo(_map[key], _context);
		} else {
			assert(_parent);
			return _parent->get(key);
		}
	}

	void add(Key key) {
		_map.insert(make_pair(key, _nextId++));
	}

	uint16_t nextId() const {
		return _nextId;
	}

private:
	uint16_t _context;
	std::map<Key, uint16_t> _map;
	VarScopeMap* _parent;
	uint16_t _nextId;
};


class CodeBuilderVisitor: public mathvm::AstVisitor {
	Code* _code;
	std::stack<BytecodeFunction*> _functions;
	std::stack<VarScopeMap*> _varScopes;
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
		return _functions.top()->bytecode();
	}

	void addVar(const AstVar* var) {
		_varScopes.top()->add(var);
	}

	VarInfo getVarInfo(const AstVar* var) {
		return _varScopes.top()->get(var);
	}
};

} /* namespace mathvm */
#endif /* CODEBUILDERVISITOR_H_ */
