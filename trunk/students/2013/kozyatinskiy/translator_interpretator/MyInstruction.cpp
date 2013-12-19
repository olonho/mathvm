#include "MyInstruction.h"

MyInstruction* MyInstruction::GetCast(VarType from, VarType to)
{
	if (from == VT_INT && to == VT_DOUBLE)
		return new ConvertTop<__int64, double>();
	if (from == VT_DOUBLE && to == VT_INT)
		return new ConvertTop<double, __int64>();
	throw std::logic_error("bad convertion");
}

MyInstruction* MyInstruction::GetStore(VarType type, const string& name, MyVarScope* varScope)
{
	if (type == VT_INT)
		return new Store<__int64>(name, varScope);
	if (type == VT_STRING)
		return new Store<string>(name, varScope);
	if (type == VT_DOUBLE)
		return new Store<double>(name, varScope);
	throw std::logic_error("bad store");
}

Integer* Integer::GetByKind(TokenKind kind)
{
	switch(kind)
	{
		case tOR:       // ||
			return new LogicIntegerOr();
        case tAND:      // &&
			return new LogicIntegerAnd();
        case tNEQ:      // !=
			return new LogicIntegerNeq();
        case tEQ:       // ==
			return new LogicIntegerEq();
		case tAOR:      // |
			return new AIntegerOr();
        case tAAND:     // &
			return new AIntegerAnd();
		case tAXOR:
			return new AIntegerXor();
        case tMOD:      // %
			return new AIntegerMod();
		default:
			throw std::logic_error("bad operation");
	}
}

Numeric* Numeric::GetByKind(TokenKind kind, VarType type)
{
	if (type == VT_INT)
	{
		switch(kind)
		{
			case tGT:       // >
				return new GT<__int64>();
			case tGE:       // >=
				return new GE<__int64>();
			case tLT:       // <
				return new LT<__int64>();
			case tLE:       // <=
				return new LE<__int64>();
			case tADD:      // +
				return new ADD<__int64>();
			case tSUB:      // -
				return new SUB<__int64>();
			case tMUL:      // *
				return new MUL<__int64>();
			case tDIV:      // /
				return new DIV<__int64>();
			default:
				throw std::logic_error("bad operation");
		}
	}

	if (type == VT_DOUBLE)
	{
		switch(kind)
		{
			case tGT:       // >
				return new GT<double>();
			case tGE:       // >=
				return new GE<double>();
			case tLT:       // <
				return new LT<double>();
			case tLE:       // <=
				return new LE<double>();
			case tADD:      // +
				return new ADD<double>();
			case tSUB:      // -
				return new SUB<double>();
			case tMUL:      // *
				return new MUL<double>();
			case tDIV:      // /
				return new DIV<double>();
			default:
				throw std::logic_error("bad operation");
		}
	}
}


