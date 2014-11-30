#ifndef UTILS_H
#define UTILS_H
#include "mathvm.h"
#include <string>
#include <sstream>
using namespace mathvm;

struct Utils
{
	static string convertToString(int value)
	{
		stringstream ss;
		ss << value;
		return std::string(ss.str());
	}

	static VarType getType(Instruction instruction)
	{
		switch (instruction)
		{
		case BC_ILOAD:
		case BC_ILOAD0:
		case BC_ILOAD1:
		case BC_ILOADM1:
		case BC_IADD:
		case BC_ISUB:
		case BC_IMUL:
		case BC_IDIV:
		case BC_IMOD:
		case BC_INEG:
		case BC_IAOR:
		case BC_IAAND:
		case BC_IAXOR:
		case BC_I2D:
		case BC_ICMP:
		case BC_LOADCTXIVAR:
		case BC_STORECTXIVAR:
			return VT_INT;
			break;

		case BC_DLOAD:
		case BC_DLOAD0:
		case BC_DLOAD1:
		case BC_DLOADM1:
		case BC_DADD:
		case BC_DSUB:
		case BC_DMUL:
		case BC_DDIV:
		case BC_DNEG:
		case BC_D2I:
		case BC_DCMP:
		case BC_LOADCTXDVAR:
		case BC_STORECTXDVAR:
			return VT_DOUBLE;
			break;

		case BC_SLOAD:
		case BC_S2I:
			return VT_STRING;

		default:
			break;
		}

		return VT_INVALID;
	}
};
#endif // UTILS_H
