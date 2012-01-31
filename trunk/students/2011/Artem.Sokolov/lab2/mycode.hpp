#ifndef _MYCODE_HPP_
#define _MYCODE_HPP_

#include "mathvm.h"

class MyCode: public mathvm::Code {
public:
	mathvm::Status* execute(std::vector<mathvm::Var*>& vars) {
		return NULL;
	}
};

#endif // _MYCODE_HPP_
