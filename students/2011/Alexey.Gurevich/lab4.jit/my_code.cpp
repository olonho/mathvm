#include "my_code.h"
#include <stdio.h>
#include <stdlib.h>
#include <AsmJit/AsmJit.h>

using namespace mathvm;
using namespace AsmJit;

MyMachCodeImpl::MyMachCodeImpl() : _code(0) {
}

MyMachCodeImpl::~MyMachCodeImpl() {
	MemoryManager::getGlobal()->free(_code);
}

Status* MyMachCodeImpl::execute(vector<Var*>& vars) {
	function_cast<int (*)()>(_code)();
	return new Status();
}

Status* MyCode::execute(vector<Var*>& vars) {
	return NULL;
}


