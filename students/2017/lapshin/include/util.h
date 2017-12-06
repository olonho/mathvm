#pragma once

#include "mathvm.h"

namespace mathvm::ldvsoft {
	enum VarTypeEx {
		VTE_INVALID = VT_INVALID,
		VTE_VOID    = VT_VOID,
		VTE_DOUBLE  = VT_DOUBLE,
		VTE_INT     = VT_INT,
		VTE_STRING  = VT_STRING,
		VTE_BOOL
	};

	using std::to_string;
	string to_string(VarTypeEx value);
	VarTypeEx extend(VarType t);
	VarTypeEx common_of(VarTypeEx a, VarTypeEx b);

	namespace StatusEx {
		Status *Error(string const &reason, uint32_t position = Status::INVALID_POSITION);
	}
};
