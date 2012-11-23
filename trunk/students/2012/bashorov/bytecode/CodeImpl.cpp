#include "CodeImpl.h"
#include "Runner.h"


namespace mathvm {

Status* CodeImpl::execute(vector<Var*>& vars) {
	return Runner(this).execute(vars);
}

}
