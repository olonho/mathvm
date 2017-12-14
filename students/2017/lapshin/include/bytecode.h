#pragma once

#include "mathvm.h"
#include "util.h"

namespace mathvm { namespace ldvsoft {

class Bytecode : public ::mathvm::Bytecode {
public:
	Bytecode() = default;
	~Bytecode() = default;

	void addAll(Bytecode const &src);
	void addInsns(initializer_list<Instruction> insns);
	
	Status *addCast(VarTypeEx from, VarTypeEx to, uint32_t position);
};

}}
