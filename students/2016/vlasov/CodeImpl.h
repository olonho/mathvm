//
// Created by svloyso on 21.11.16.
//

#ifndef MATHVM_BYTECODE_H
#define MATHVM_BYTECODE_H

#include "mathvm.h"

namespace mathvm {

class CodeImpl : public Code {
public:
	virtual Status* execute(vector<Var*>& vars) {
		return Status::Error("Not implemented");
	}
};

} // namespace mathvm


#endif //MATHVM_BYTECODE_H
