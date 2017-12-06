#pragma once

#include "mathvm.h"

namespace mathvm::ldvsoft {
	enum class VarTypeEx {
		INVALID = VT_INVALID,
		VOID    = VT_VOID,
		DOUBLE  = VT_DOUBLE,
		INT     = VT_INT,
		STRING  = VT_STRING,
		BOOL
	};

	using std::to_string;
	string to_string(VarTypeEx value);	
	VarTypeEx extend(VarType t);
	VarTypeEx common_of(VarTypeEx a, VarTypeEx b);

	string escape(string const &s);

	namespace StatusEx {
		Status *Error(string const &reason, uint32_t position = Status::INVALID_POSITION);
	}
};
