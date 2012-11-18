/*
 * ExecutableCode.h
 *
 *  Created on: Oct 25, 2012
 *      Author: alex
 */

#ifndef EXECUTABLECODE_H_
#define EXECUTABLECODE_H_

#include "mathvm.h"

namespace mathvm {

class Context {
	Context* _parent;
	uint16_t _id;
	vector<Var*> _variables;
public:
	Context(Context* parent, TranslatedFunction* function)
			: _parent(parent), _id(function->id()) {
		_variables.reserve(function->localsNumber());
	}

	~Context() {
		while (_variables.size() > 0) {
			delete _variables.back();
			_variables.pop_back();
		}
	}

	Context* getContext(uint16_t id) {
		if (id == _id) {
			return this;
		}

		// TODO: remove, debug only.
		if (_parent == 0) {
			cout << "null context parent for " << id << endl;
			return 0;
		}

		return _parent->getContext(id);
	}

	Var* getVar(uint16_t index) {
		assert(_variables.size() > index);
		return _variables[index];
	}

//	void setVar(uint16_t index, Var& var) {
//		if (index() > _variables.size()) {
//			_variables.reserve(index + 1);
//		}
//		_variables[index] = var;
//	}

//	uint16_t addVar(const Var& var) {
//		_variables.push_back(var);
//		return _variables.size() - 1;
//	}

	uint16_t variables() const {
		return _variables.size();
	}
};

class ExecutableCode: public Code {
public:
	ExecutableCode();
	virtual ~ExecutableCode();
	virtual Status* execute(vector<Var*>& vars);

private:
	uint16_t _bp;
	uint16_t _ip;
	Context* _context;

};

} /* namespace mathvm */
#endif /* EXECUTABLECODE_H_ */
