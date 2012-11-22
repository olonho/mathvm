#include "CodeImpl.h"

namespace mathvm {

Status* CodeImpl::execute(vector<Var*>&) {
	FunctionIterator funcIt(this);
	while (funcIt.hasNext()) {
		TranslatedFunction* fun = funcIt.next();
		Bytecode* bytecode = fun->bytecode();

		#define readInstruction() bytecode->getInsn(ip);

		uint32_t ip = 0
		while (ip < bytecode->length()) {
			Instruction instruction = bytecode->getInsn(ip);
			switch (instruction) {
				case BC_DPRINT:

				case BC_IPRINT: 
				case BC_SPRINT: 
				case BC_DLOAD: 
				case BC_ILOAD: 
				case BC_SLOAD: 
				case BC_DLOAD0: 
				case BC_ILOAD0: 
				case BC_SLOAD0: 
				case BC_DLOAD1: 
				case BC_ILOAD1: 
				case BC_SLOAD1: 
				case BC_DLOADM1: 
				case BC_ILOADM1: 
				case BC_SLOADM1: 
			}
		}
	}
}

}
