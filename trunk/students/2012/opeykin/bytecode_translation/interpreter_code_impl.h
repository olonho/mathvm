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

union ContextVar {
    double _doubleValue;
    int64_t _intValue;
    uint16_t _stringId;
};

class Context {
	Context* _parent;
	uint16_t _ip;
	BytecodeFunction* _function;
	vector<ContextVar> _variables;
public:
	Context(Context* parent, BytecodeFunction* function)
			: _parent(parent), _ip(0), _function(function),
			  _variables(function->localsNumber() + function->parametersNumber()) {
	}

	~Context() {
	}

	uint16_t ip() const {
		return _ip;
	}

	void setIp(uint16_t ip) {
		_ip = ip;
	}

	Bytecode* bytecode() {
		return _function->bytecode();
	}

	Context* parent() {
		return _parent;
	}

	Context* getContext(uint16_t id) {
		if (id == _function->id()) {
			return this;
		}

		// TODO: remove, debug only.
		if (_parent == 0) {
			cout << "null context parent for " << id << endl;
			return 0;
		}

		return _parent->getContext(id);
	}

	ContextVar* getVar(uint16_t index) {
		assert(_variables.size() > index);
		return &_variables[index];
	}

	uint16_t variables() const {
		return _variables.size();
	}
};

class InterpreterCodeImpl: public mathvm::Code {
public:
	InterpreterCodeImpl(ostream& out);
	virtual ~InterpreterCodeImpl();
    BytecodeFunction* bytecodeFunctionById(uint16_t index) const;
    BytecodeFunction* bytecodeFunctionByName(const string& name) const;
    virtual Status* execute(vector<Var*>& vars);

private:
    uint16_t nextUInt16();
    int64_t nextInt();
    double readDoubleFromBytecode();

    int64_t getIntFromTOS();
    double getDoubleFromTOS();
    uint16_t getStringIdFromTOS();
    void pushIntToTOS(int value);
    void pushDoubleToTOS(double value);

//    void loadIntVar(uint32_t index);
//    void loadDoubleVar(uint32_t index);
    void loadVar(uint32_t index);
    void storeIntVar(uint32_t index);
    void storeStringVar(uint32_t index);
    void storeDoubleVar(uint32_t index);
    void callFunction(uint32_t id);

    template<class Comparator>
    void jump(Comparator comparator) {
    	if (comparator(getIntFromTOS(), getIntFromTOS())) {
    		_ip += _bp->getInt16(_ip);
    	} else {
    		_ip += 2;
    	}
    }

    void returnFromFunction();

private:

    std::stack<ContextVar> _stack;
	Bytecode* _bp;
	uint16_t _ip;
	Context* _context;
	ostream& _out;
};

} /* namespace mathvm */
#endif /* INTERPRETER_CODE_IMPL_H_ */
