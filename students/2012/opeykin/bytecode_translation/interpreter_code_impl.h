/*
 * interpreter_code_impl.h
 *
 *  Created on: Nov 20, 2012
 *      Author: alex
 */

#ifndef INTERPRETER_CODE_IMPL_H_
#define INTERPRETER_CODE_IMPL_H_

#include "mathvm.h"
#include <stack>

namespace mathvm {

class Context {
	Context* _parent;
	uint16_t _id;
	vector<Var*> _variables;
public:
	Context(Context* parent, TranslatedFunction* function)
			: _parent(parent), _id(function->id()) {
		for (uint32_t i = 0; i < function->localsNumber(); ++i) {
			VarType type = function->parameterType(i);
			string name = function->parameterName(i);
			_variables.push_back(new Var(type, name));
		}
	}

	~Context() {
		while (!_variables.empty()) {
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

	uint16_t variables() const {
		return _variables.size();
	}
};

class InterpreterCodeImpl: public mathvm::Code {
public:
	InterpreterCodeImpl();
	virtual ~InterpreterCodeImpl();
    BytecodeFunction* bytecodeFunctionById(uint16_t index) const;
    BytecodeFunction* bytecodeFunctionByName(const string& name) const;
    virtual Status* execute(vector<Var*>& vars);

private:
    uint16_t readUInt16FromBytecode();
    int readIntFromBytecode();
    double readDoubleFromBytecode();

    int getIntFromTOS();
    double getDoubleFromTOS();
    void pushIntToTOS(int value);
    void pushDoubleToTOS(double value);

    void loadIntVar(uint32_t index);
    void loadDoubleVar(uint32_t index);
    void storeIntVar(uint32_t index);
    void storeDoubleVar(uint32_t index);

    void jump();

private:

    union StackUnit {
        double _doubleValue;
        int64_t _intValue;
        const char* _stringValue;
    };

    std::stack<StackUnit> _stack;
	Bytecode* _bp;
	uint16_t _ip;
	Context* _context;
	ostream& _out;
};

} /* namespace mathvm */
#endif /* INTERPRETER_CODE_IMPL_H_ */
