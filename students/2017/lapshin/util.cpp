#include "util.h"

#include <sstream>

namespace mathvm::ldvsoft {

string escape(string const &s) {
	stringstream ss;
	ss << '\'';
	for (auto c: s)
		switch (c) {
		case '\'':
			ss << "\\'";
			break;
		case '\a':
			ss << "\\a";
			break;
		case '\b':
			ss << "\\b";
			break;
		case '\f':
			ss << "\\f";
			break;
		case '\n':
			ss << "\\n";
			break;
		case '\r':
			ss << "\\r";
			break;
		case '\t':
			ss << "\\t";
			break;
		case '\v':
			ss << "\\v";
			break;
		default:
			ss << c;
		}
	ss << '\'';
	return ss.str();
}

string to_string(VarTypeEx value) {
	switch (value) {
	case VarTypeEx::INVALID: return "INVALID";
	case VarTypeEx::VOID   : return "void";
	case VarTypeEx::DOUBLE : return "double";
	case VarTypeEx::INT    : return "int";
	case VarTypeEx::BOOL   : return "bool+int";
	case VarTypeEx::STRING : return "string";
	}
	__builtin_unreachable();
}

VarTypeEx extend(VarType t) {
	return static_cast<VarTypeEx>(static_cast<uint8_t>(t));
}

VarTypeEx common_of(VarTypeEx a, VarTypeEx b) {
	if (a == VarTypeEx::VOID || b == VarTypeEx::VOID)
		return VarTypeEx::INVALID;
	if (a == b)
		return a;
	if (a == VarTypeEx::DOUBLE || b == VarTypeEx::DOUBLE)
		return VarTypeEx::DOUBLE;
	if (a == VarTypeEx::INT || b == VarTypeEx::INT)
		return VarTypeEx::INT;
	if (a == VarTypeEx::BOOL || b == VarTypeEx::BOOL)
		return VarTypeEx::BOOL;
	return VarTypeEx::STRING;
}

Status* StatusEx::Error(string const &reason, uint32_t position) {
	return Status::Error(reason.c_str(), position);
}

}
