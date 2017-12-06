#include "code.h"

namespace mathvm::ldvsoft {

Status *BytecodeCode::execute(vector<Var*> &vars) {
	static_cast<void>(vars);
	return Status::Error("Execution is not supported yet");
}

}
