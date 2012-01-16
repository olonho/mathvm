#pragma once
#include "ast.h"

namespace mathvm {

class MyCode : public Code {
public:
	MyCode() {}
	~MyCode() {}
	Status* execute(vector<Var*>& vars) {
		// don't need to implement it for lab2
		return new Status();
	}
};

class MyTranslator : public Translator {
public:
	MyTranslator() {}
	~MyTranslator() {}
	Status* translate(const string& program, Code* *code);
};

}
