#include "util.h"

#include <sstream>

namespace mathvm::ldvsoft {

VarType VarEx::type() const {
	switch (storage.index()) {
	case 0:  return VT_DOUBLE;
	case 1:  return VT_INT;
	case 2:  return VT_STRING;
	default: return VT_INVALID;
	}
}

VarEx::VarEx(Var const &v) {
	switch (v.type()) {
	case VT_DOUBLE:
		storage.emplace<double>(v.getDoubleValue());
		break;
	case VT_INT:
		storage.emplace<int64_t>(v.getIntValue());
		break;
	case VT_STRING:
		storage.emplace<string>(v.getStringValue());
		break;
	default:
		;
	}
}

VarEx::VarEx(double d):        storage{d} {}
VarEx::VarEx(int64_t i):       storage{i} {}
VarEx::VarEx(string const &s): storage{s} {}

Var &assign(Var &target, VarEx const &source) {
	switch (source.storage.index()) {
	case 0:
		target.setDoubleValue(std::get<0>(source.storage));
		break;
	case 1:
		target.setDoubleValue(std::get<0>(source.storage));
		break;
	case 2:
		target.setDoubleValue(std::get<0>(source.storage));
		break;
	}
	return target;
}

ostream &operator<<(ostream &out, VarEx const &v) {
	switch (v.type()) {
	case VT_DOUBLE:
		return out << v.d();
	case VT_INT:
		return out << v.i();
	case VT_STRING:
		return out << v.s();
	default:
		;
	}
	return out;
}

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

VarType shrink(VarTypeEx t) {
	if (t == VarTypeEx::BOOL)
		return VT_INT;
	return static_cast<VarType>(static_cast<uint8_t>(t));
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
