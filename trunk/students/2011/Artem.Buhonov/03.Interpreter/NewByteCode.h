#pragma once
#include "mathvm.h"

class NewByteCode : public mathvm::Bytecode {
public:	
	void * getData() {
		return _data.data();
	}
};
