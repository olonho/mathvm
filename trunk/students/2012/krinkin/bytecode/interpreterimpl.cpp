#include "interpreterimpl.h"

#include "ast.h"

#include <iostream>
#include <cstring>
#include <cassert>

void InterpreterImpl::run()
{
	m_locals.resize(count_functions(), 0);
	m_iregisters.resize(4, (int64_t)0);
	m_dregisters.resize(4, 0.0);
	m_sregisters.resize(4, (char const *)0);
	call( (BytecodeFunction *) m_code->functionByName(AstFunction::top_name) );
	clear();
}

static size_t bclen(Instruction insn) {
	static const struct
	{
		const char* name;
		Instruction insn;
		size_t length;
	} names[] = {
		#define BC_NAME(b, d, l) {#b, BC_##b, l},
		FOR_BYTECODES(BC_NAME)
	};

	if (insn >= BC_INVALID && insn < BC_LAST) return names[insn].length;

	assert(false);
	return 0;
}

void InterpreterImpl::call(BytecodeFunction *fun)
{
	char *old_locals = m_locals[fun->id()];
	
	if (fun->localsNumber())
	{
		m_locals[fun->id()] = new char[fun->localsNumber()];
		memset(m_locals[fun->id()], 0, fun->localsNumber());
	}
	else m_locals[fun->id()] = (char *) 0;
	
	Bytecode *bytecode = fun->bytecode();
	
	int64_t itemp;
	double dtemp;
	char const * stemp;
	//size_t stack_size;
	TranslatedFunction *called_function;
	
	for (size_t bci = 0; bci < bytecode->length();)
	{
		Instruction insn = bytecode->getInsn(bci);
		if (jump(insn))
		{
			if (insn != BC_JA)
			{
				int64_t upper = m_stack.top().ival; m_stack.pop();
				int64_t lower = m_stack.top().ival; m_stack.push(upper);
			
				switch (insn)
				{
				case BC_IFICMPNE:
					if (upper != lower) bci += bytecode->getInt16(bci + 1) + 1;
					else bci += bclen(insn);
					break;
				case BC_IFICMPE:
					if (upper == lower) bci += bytecode->getInt16(bci + 1) + 1;
					else bci += bclen(insn);
					break;
				case BC_IFICMPG:
					if (upper > lower) bci += bytecode->getInt16(bci + 1) + 1;
					else bci += bclen(insn);
					break;
				case BC_IFICMPGE:
					if (upper >= lower) bci += bytecode->getInt16(bci + 1) + 1;
					else bci += bclen(insn);
					break;
				case BC_IFICMPL:
					if (upper < lower) bci += bytecode->getInt16(bci + 1) + 1;
					else bci += bclen(insn);
					break;
				case BC_IFICMPLE:
					if (upper <= lower) bci += bytecode->getInt16(bci + 1) + 1;
					else bci += bclen(insn);
					break;
				default:
						assert(0);
				}
			}
			else
			{
				bci += bytecode->getInt16(bci + 1) + 1;
			}
		}
		else
		{
			size_t length = bclen(insn);
			if (one_argument(insn))
			{
				stack_value tos = m_stack.top(); m_stack.pop();
				switch (insn)
				{
				case BC_DNEG:
					m_stack.push(-tos.dval);
					break;
				case BC_INEG:
					m_stack.push(-tos.ival);
					break;
				case BC_IPRINT:
					std::cout << tos.ival;
					break;
				case BC_DPRINT:
					std::cout << tos.dval;
					break;
				case BC_SPRINT:
					std::cout << tos.sval;
					break;
				case BC_I2D:
					m_stack.push((double)tos.ival);
					break;
				case BC_D2I:
					m_stack.push((int64_t)tos.dval);
					break;
				case BC_POP:
					break;
				case BC_STOREDVAR0:
					m_dregisters[0] = tos.dval;
					break;
				case BC_STOREDVAR1:
					m_dregisters[1] = tos.dval;
					break;
				case BC_STOREDVAR2:
					m_dregisters[2] = tos.dval;
					break;
				case BC_STOREDVAR3:
					m_dregisters[3] = tos.dval;
					break;
				case BC_STOREIVAR0:
					m_iregisters[0] = tos.ival;
					break;
				case BC_STOREIVAR1:
					m_iregisters[1] = tos.ival;
					break;
				case BC_STOREIVAR2:
					m_iregisters[2] = tos.ival;
					break;
				case BC_STOREIVAR3:
					m_iregisters[3] = tos.ival;
					break;
				case BC_STORESVAR0:
					m_sregisters[0] = tos.sval;
					break;
				case BC_STORESVAR1:
					m_sregisters[1] = tos.sval;
					break;
				case BC_STORESVAR2:
					m_sregisters[2] = tos.sval;
					break;
				case BC_STORESVAR3:
					m_sregisters[3] = tos.sval;
					break;
				case BC_STOREDVAR:
					store_value_at(tos.dval, fun->id(), bytecode->getUInt16(bci + 1));
					break;
				case BC_STOREIVAR:
					store_value_at(tos.ival, fun->id(), bytecode->getUInt16(bci + 1));
					break;
				case BC_STORESVAR:
					store_value_at(tos.sval, fun->id(), bytecode->getUInt16(bci + 1));
					break;
				case BC_STORECTXDVAR:
					store_value_at(tos.dval, bytecode->getUInt16(bci + 1), bytecode->getUInt16(bci + 3));
					break;
				case BC_STORECTXIVAR:
					store_value_at(tos.ival, bytecode->getUInt16(bci + 1), bytecode->getUInt16(bci + 3));
					break;
				case BC_STORECTXSVAR:
					store_value_at(tos.sval, bytecode->getUInt16(bci + 1), bytecode->getUInt16(bci + 3));
					break;
				default: assert(0);
				}
			}
			else if (two_arguments(insn))
			{
				stack_value upper = m_stack.top(); m_stack.pop();
				stack_value lower = m_stack.top(); m_stack.pop();
				switch (insn)
				{
				case BC_DADD:
					m_stack.push(upper.dval + lower.dval);
					break;
				case BC_IADD:
					m_stack.push(upper.ival + lower.ival);
					break;
				case BC_DSUB:
					m_stack.push(upper.dval - lower.dval);
					break;
				case BC_ISUB:
					m_stack.push(upper.ival - lower.ival);
					break;
				case BC_DMUL:
					m_stack.push(upper.dval * lower.dval);
					break;
				case BC_IMUL:
					m_stack.push(upper.ival * lower.ival);
					break;
				case BC_DDIV:
					m_stack.push(upper.dval / lower.dval);
					break;
				case BC_IDIV:
					m_stack.push(upper.ival / lower.ival);
					break;
				case BC_IMOD:
					m_stack.push(upper.ival / lower.ival);
					break;
				case BC_SWAP:
					m_stack.push(upper);
					m_stack.push(lower);
					break;
				case BC_DCMP:
					if (upper.dval < lower.dval) m_stack.push((int64_t)-1);
					else if (upper.dval > lower.dval) m_stack.push((int64_t)1);
					else m_stack.push((int64_t)0);
					break;
				case BC_ICMP:
					if (upper.ival < lower.ival) m_stack.push((int64_t)-1);
					else if (upper.ival > lower.ival) m_stack.push((int64_t)1);
					else m_stack.push((int64_t)0);
					break;
				default:
					assert(0);
				}
			}
			else
			{
				switch (insn)
				{
				case BC_DLOAD:
					m_stack.push(bytecode->getDouble(bci + 1));
					break;
				case BC_ILOAD:
					m_stack.push(bytecode->getInt64(bci + 1));
					break;
				case BC_SLOAD:
					m_stack.push(m_code->constantById(bytecode->getUInt16(bci + 1)).c_str());
					break;
				case BC_DLOAD0:
					m_stack.push(0.0);
					break;
				case BC_ILOAD0:
					m_stack.push((int64_t)0);
					break;
				case BC_SLOAD0:
					m_stack.push(empty.c_str());
					break;
				case BC_DLOAD1:
					m_stack.push(1.0);
					break;
				case BC_ILOAD1:
					m_stack.push((int64_t)1);
					break;
				case BC_DLOADM1:
					m_stack.push(-1.0);
					break;
				case BC_ILOADM1:
					m_stack.push((int64_t)-1);
					break;
				case BC_LOADDVAR0:
					m_stack.push(m_dregisters[0]);
					break;
				case BC_LOADDVAR1:
					m_stack.push(m_dregisters[1]);
					break;
				case BC_LOADDVAR2:
					m_stack.push(m_dregisters[2]);
					break;
				case BC_LOADDVAR3:
					m_stack.push(m_dregisters[3]);
					break;
				case BC_LOADIVAR0:
					m_stack.push(m_iregisters[0]);
					break;
				case BC_LOADIVAR1:
					m_stack.push(m_iregisters[1]);
					break;
				case BC_LOADIVAR2:
					m_stack.push(m_iregisters[2]);
					break;
				case BC_LOADIVAR3:
					m_stack.push(m_iregisters[3]);
					break;
				case BC_LOADSVAR0:
					m_stack.push(m_sregisters[0]);
					break;
				case BC_LOADSVAR1:
					m_stack.push(m_sregisters[1]);
					break;
				case BC_LOADSVAR2:
					m_stack.push(m_sregisters[2]);
					break;
				case BC_LOADSVAR3:
					m_stack.push(m_sregisters[3]);
					break;
				case BC_LOADDVAR:
					load_value_from(dtemp, fun->id(), bytecode->getUInt16(bci + 1));
					m_stack.push(dtemp);
					break;
				case BC_LOADIVAR:
					load_value_from(itemp, fun->id(), bytecode->getUInt16(bci + 1));
					m_stack.push(itemp);
					break;
				case BC_LOADSVAR:
					load_value_from(stemp, fun->id(), bytecode->getUInt16(bci + 1));
					m_stack.push(stemp);
					break;
				case BC_LOADCTXDVAR:
					load_value_from(dtemp, bytecode->getUInt16(bci + 1), bytecode->getUInt16(bci + 3));
					m_stack.push(dtemp);
					break;
				case BC_LOADCTXIVAR:
					load_value_from(itemp, bytecode->getUInt16(bci + 1), bytecode->getUInt16(bci + 3));
					m_stack.push(itemp);
					break;
				case BC_LOADCTXSVAR:
					load_value_from(stemp, bytecode->getUInt16(bci + 1), bytecode->getUInt16(bci + 3));
					m_stack.push(stemp);
					break;
				case BC_CALL:
					called_function = m_code->functionById(bytecode->getUInt16(bci + 1));
					//stack_size = m_stack.size();
					//if (called_function->returnType() != VT_VOID) stack_size += 1;
					call((BytecodeFunction *)called_function);
					//while (m_stack.size() > stack_size) m_stack.pop();
					break;
				case BC_RETURN:
					delete m_locals[fun->id()];
					m_locals[fun->id()] = old_locals;
					return;
				default:
					assert(0);
				}
			}
			bci += length;
		}
	}
	delete m_locals[fun->id()];
	m_locals[fun->id()] = old_locals;
}

void InterpreterImpl::store_value_at(int64_t value, uint16_t scope, uint16_t offset)
{
	assert(m_locals[scope]);
	int64_t *buf = (int64_t *)(m_locals[scope] + offset);
	*buf = value;
}

void InterpreterImpl::store_value_at(double value, uint16_t scope, uint16_t offset)
{
	assert(m_locals[scope]);
	double *buf = (double *)(m_locals[scope] + offset);
	*buf = value;
}

void InterpreterImpl::store_value_at(char const *value, uint16_t scope, uint16_t offset)
{
	assert(m_locals[scope]);
	char const **buf = (char const **)(m_locals[scope] + offset);
	*buf = value;
}

void InterpreterImpl::load_value_from(int64_t &value, uint16_t scope, uint16_t offset)
{
	assert(m_locals[scope]);
	int64_t *buf = (int64_t *)(m_locals[scope] + offset);
	value = *buf;
}

void InterpreterImpl::load_value_from(double &value, uint16_t scope, uint16_t offset)
{
	assert(m_locals[scope]);
	double *buf = (double *)(m_locals[scope] + offset);
	value = *buf;
}

void InterpreterImpl::load_value_from(char const * &value, uint16_t scope, uint16_t offset)
{
	assert(m_locals[scope]);	
	char const **buf = (char const **)(m_locals[scope] + offset);
	value = *buf;
}

bool InterpreterImpl::jump(Instruction insn) const
{
	switch (insn)
	{
		case BC_IFICMPNE: case BC_IFICMPE:
		case BC_IFICMPG: case BC_IFICMPGE:
		case BC_IFICMPL: case BC_IFICMPLE:
		case BC_JA:	
			return true;
		default:
			return false;
	}
}

bool InterpreterImpl::one_argument(Instruction insn) const
{
	switch (insn)
	{
	case BC_DNEG: case BC_INEG:
	case BC_IPRINT: case BC_DPRINT: case BC_SPRINT:
	case BC_I2D: case BC_D2I:
	case BC_POP:
	case BC_STOREDVAR0: case BC_STOREDVAR1: case BC_STOREDVAR2: case BC_STOREDVAR3:
	case BC_STOREIVAR0: case BC_STOREIVAR1: case BC_STOREIVAR2: case BC_STOREIVAR3:
	case BC_STORESVAR0: case BC_STORESVAR1: case BC_STORESVAR2: case BC_STORESVAR3:
	case BC_STOREDVAR: case BC_STOREIVAR: case BC_STORESVAR:
	case BC_STORECTXDVAR: case BC_STORECTXIVAR: case BC_STORECTXSVAR:
		return true;
	default:
		return false;
	}
}

bool InterpreterImpl::two_arguments(Instruction insn) const
{
	switch (insn)
	{
	case BC_DADD: case BC_IADD:
	case BC_DSUB: case BC_ISUB:
	case BC_DMUL: case BC_IMUL:
	case BC_DDIV: case BC_IDIV:
	case BC_IMOD:
	case BC_SWAP:
	case BC_DCMP: case BC_ICMP:
		return true;
	default:
		return false;
	}
}

size_t InterpreterImpl::count_functions() const
{
	size_t count = 0;
	Code::FunctionIterator it(m_code);
	while (it.next()) ++count;
	
	return count;
}

void InterpreterImpl::clear()
{
	for (std::vector<char *>::const_iterator it = m_locals.begin(); it != m_locals.end(); ++it)
	{
		if (*it) delete *it;
	}
}
