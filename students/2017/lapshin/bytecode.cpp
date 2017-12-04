#include "bytecode.h"

#include "util.h"

namespace mathvm::ldvsoft {

void Bytecode::addAll(Bytecode const &src) {
	_data.insert(_data.end(), src._data.begin(), src._data.end());
}

void Bytecode::addInsns(initializer_list<Instruction> insns) {
	_data.insert(_data.end(), insns.begin(), insns.end());
}

Status *Bytecode::addCast(VarTypeEx from, VarTypeEx to, uint32_t position) {
	assert(from != VTE_INVALID || to != VTE_INVALID);
	if (from == to)
		return Status::Ok();
	if (to == VTE_VOID) {
		addInsn(BC_POP);
		return Status::Ok();
	}
	if (from == VTE_VOID)
		return StatusEx::Error("Cannot cast from void to " + to_string(to), position);
	if (to == VTE_STRING)
		return StatusEx::Error("Cannot cast from " + to_string(from) + " to string", position);

	if (from == VTE_STRING) {
		addInsn(BC_S2I);
		from = VTE_INT;
	}

	if (to == VTE_BOOL) {
		auto s{addCast(from, VTE_INT, position)};
		if (!s->isOk())
			return s;
		delete s;

		// fun (x: Int): Bool = (x <=> 0) & 1
		addInsns({
			BC_ILOAD0,
			BC_ICMP,
			BC_ILOAD1,
			BC_IAAND
		});
		return Status::Ok();
	}
	if (from == VTE_BOOL)
		from = VTE_INT;

	if (from == VTE_INT)
		addInsn(BC_I2D);
	else
		addInsn(BC_D2I);
	return Status::Ok();
}

}
