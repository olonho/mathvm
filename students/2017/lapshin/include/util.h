#pragma once

#include "mathvm.h"

#include <variant>

namespace mathvm::ldvsoft {
	class VarEx {
	public:
		std::variant<double, int64_t, char const *> storage;

		VarType type() const;
		VarEx(Var const &v);
		VarEx(double d);
		VarEx(int64_t i = 0);
		VarEx(char const *s);

		friend Var &assign(Var &target, VarEx const &source);

		int64_t const &i() const {
			return std::get<int64_t>(storage);
		}
		double const &d() const {
			return std::get<double>(storage);
		}
		char const *s() const {
			return std::get<char const *>(storage);
		}

		friend ostream &operator<<(ostream &out, VarEx const &v);
	};

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
	VarType shrink(VarTypeEx t);
	VarTypeEx common_of(VarTypeEx a, VarTypeEx b);

	string escape(string const &s);

	namespace StatusEx {
		Status *Error(string const &reason, uint32_t position = Status::INVALID_POSITION);
	}
}
