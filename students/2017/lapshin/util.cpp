#include "util.h"

namespace mathvm::ldvsoft {

string to_string(VarTypeEx value) {
	switch (value) {
	case VTE_INVALID: return "INVALID";
	case VTE_VOID   : return "void";
	case VTE_DOUBLE : return "double";
	case VTE_INT    : return "int";
	case VTE_BOOL   : return "bool+int";
	case VTE_STRING : return "string";
	}
	__builtin_unreachable();
}

VarTypeEx extend(VarType t) {
	return static_cast<VarTypeEx>(static_cast<uint8_t>(t));
}

VarTypeEx common_of(VarTypeEx a, VarTypeEx b) {
	if (a == VTE_VOID || b == VTE_VOID)
		return VTE_INVALID; 
	if (a == b)
		return a;
	if (a == VTE_DOUBLE || b == VTE_DOUBLE)
		return VTE_DOUBLE;
	if (a == VTE_INT || b == VTE_INT)
		return VTE_INT;
	if (a == VTE_BOOL || b == VTE_BOOL)
		return VTE_BOOL;
	return VTE_STRING;
}

Status* StatusEx::Error(string const &reason, uint32_t position) {
	return Status::Error(reason.c_str(), position);
}

}
