#pragma once
#include "ast.h"
#include "my_code.h"

namespace mathvm {

class MyTranslator : public Translator {
public:
	MyTranslator() {}
	~MyTranslator() {}
	Status* translate(const string& program, Code* *code);
};

}
