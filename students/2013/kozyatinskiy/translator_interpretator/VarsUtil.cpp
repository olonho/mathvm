#include "VarsUtil.h"

#include <stdexcept>

void load0(Bytecode* bytecode, VarType t)
{
	switch (t)
	{
	case VT_DOUBLE:
		bytecode->add(BC_DLOAD0);
		break;
	case VT_INT:
		bytecode->add(BC_ILOAD0);
		break;
	case VT_STRING:
		bytecode->add(BC_SLOAD0);
		break;
	default:
		assert(true);
	}
}


void convert(Bytecode* bytecode, VarType from, VarType to)
{
	if (from == to) return;
	if (from == VT_INT && to == VT_DOUBLE)
		bytecode->add(BC_I2D);
	else if (from == VT_DOUBLE && to == VT_INT)
		bytecode->add(BC_D2I);
	else
		throw std::invalid_argument("bad cast");
		//assert(true);
}


void loadVar(Bytecode* bytecode, VarType t, uint32_t offset)
{
	switch (t)
	{
	case VT_DOUBLE:
		bytecode->add(BC_LOADCTXDVAR);
		break;
	case VT_INT:
		bytecode->add(BC_LOADCTXIVAR);
		break;
	case VT_STRING:
		bytecode->add(BC_LOADCTXSVAR);
		break;
	default:
		assert(true);
	}
	bytecode->addInt32(offset);
}


void print(Bytecode* bytecode, VarType t)
{
	switch (t)
	{
	case VT_DOUBLE:
		bytecode->add(BC_DPRINT);
		break;
	case VT_INT:
		bytecode->add(BC_IPRINT);
		break;
	case VT_STRING:
		bytecode->add(BC_SPRINT);
		break;
	default:
		assert(true);
	}
}


void popToVar0(Bytecode* bytecode, VarType t)
{
	switch (t)
	{
	case VT_DOUBLE:
		bytecode->add(BC_STOREDVAR0);
		break;
	case VT_INT:
		bytecode->add(BC_STOREIVAR0);
		break;
	case VT_STRING:
		bytecode->add(BC_STORESVAR0);
		break;
	default:
		assert(true);
	}
}


void pushFromVar0(Bytecode* bytecode, VarType t)
{
	switch (t)
	{
	case VT_DOUBLE:
		bytecode->add(BC_LOADDVAR0);
		break;
	case VT_INT:
		bytecode->add(BC_LOADIVAR0);
		break;
	case VT_STRING:
		bytecode->add(BC_LOADSVAR0);
		break;
	default:
		assert(true);
	}
}


void storeVar(Bytecode* bytecode, VarType t, uint32_t offset)
{
	switch (t)
	{
	case VT_DOUBLE:
		bytecode->add(BC_STORECTXDVAR);
		break;
	case VT_INT:
		bytecode->add(BC_STORECTXIVAR);
		break;
	case VT_STRING:
		bytecode->add(BC_STORECTXSVAR);
		break;
	default:
		assert(true);
	}
	bytecode->addInt32(offset);
}


int sizeOfType(VarType t)
{
	switch (t)
	{
	case VT_DOUBLE:
		return sizeof(double);
	case VT_INT:
		return sizeof(int64_t);
	case VT_STRING:
		return PointerSize;
	default:
		assert(true);
	}
	return 0;
}
