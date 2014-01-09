#ifndef BCFUNCTION_H_
#define BCFUNCTION_H_

#include "mathvm.h"
#include "ast.h"

namespace mathvm {

struct BCBytecode: Bytecode {
	uint8_t* bytecode() {
		return _data.data();
	}
};

class BCFunction: public TranslatedFunction {
public:
	BCFunction(AstFunction* f) :
			TranslatedFunction(f) {
	}
	uint32_t freeVarsNumber() const {
		return _freeVarsNumber;
	}
	void setFreeVarsNumber(uint32_t f) {
		_freeVarsNumber = f;
	}
	BCBytecode* bytecode() {
		return &_bytecode;
	}
	void disassemble(std::ostream& out) const {
		_bytecode.dump(out);
	}
private:
	BCBytecode _bytecode;
	uint32_t _freeVarsNumber;
};

} /* namespace mathvm */

#endif
