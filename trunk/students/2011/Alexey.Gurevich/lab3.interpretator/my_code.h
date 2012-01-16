#pragma once

#include "ast.h"
#include <vector>
#include <map>

namespace mathvm {

class MyCode : public Code {

	union StackValue {
		double  	doubleValue;
		int64_t 	intValue;
		const char* stringValue;
	};

	void pushDouble (double);
	void pushInt    (int64_t);
	void pushString (const char*);
	void pushValue  (StackValue);

	double   	 popDouble();
	int64_t  	 popInt();
	const char*  popString();
	StackValue   popValue();

	// for functions handling
	Status* executeBytecode(Bytecode* bytecode);

public:
	MyCode() {}
	~MyCode() {
		// TODO: free varMap_;
	}
	Status* execute(vector<Var*>& vars);

private:
	vector<StackValue>  stack_;
	map<uint16_t, Var*> varMap_;
};

}
