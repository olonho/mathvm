#ifndef _MATHVM_INTERPRETER_CODE_IMPL
#define _MATHVM_INTERPRETER_CODE_IMPL

using namespace mathvm;

class InterpreterCodeImpl: public Code {

	void removeFunction(TranslatedFunction* function);

public:

	Status* execute(vector<Var*>& vars);
};

#endif