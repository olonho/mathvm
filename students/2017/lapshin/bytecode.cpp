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
	assert(from != VarTypeEx::INVALID || to != VarTypeEx::INVALID);
	if (from == to)
		return Status::Ok();
	if (to == VarTypeEx::VOID) {
		addInsn(BC_POP);
		return Status::Ok();
	}
	if (from == VarTypeEx::VOID)
		return StatusEx::Error("Cannot cast from void to " + to_string(to), position);
	if (to == VarTypeEx::STRING)
		return StatusEx::Error("Cannot cast from " + to_string(from) + " to string", position);

	if (from == VarTypeEx::STRING) {
		addInsn(BC_S2I);
		from = VarTypeEx::INT;
	}

	if (to == VarTypeEx::BOOL) {
		auto s{addCast(from, VarTypeEx::INT, position)};
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
	if (from == VarTypeEx::BOOL)
		from = VarTypeEx::INT;

	if (from == VarTypeEx::INT)
		addInsn(BC_I2D);
	else
		addInsn(BC_D2I);
	return Status::Ok();
}

}
