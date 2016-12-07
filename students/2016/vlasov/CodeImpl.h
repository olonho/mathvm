//
// Created by svloyso on 21.11.16.
//

#ifndef MATHVM_BYTECODE_H
#define MATHVM_BYTECODE_H

#include "mathvm.h"
#include <map>
#include <stack>

namespace mathvm {

class CodeImpl : public Code {
	typedef std::map<uint16_t, std::stack<std::map<uint16_t, Var*>>> Context;
public:
	virtual Status* execute(vector<Var*>& vars);
	Var* execute(TranslatedFunction* func, Context& context);
};

} // namespace mathvm


#endif //MATHVM_BYTECODE_H
