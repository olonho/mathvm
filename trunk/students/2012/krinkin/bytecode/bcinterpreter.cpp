#include "bcinterpreter.h"

#include <AsmJit/Assembler.h>

using namespace AsmJit;

void BCInterpreter::call(BytecodeFunction *function)
{
    Bytecode *bytecode = function->bytecode();
	for (size_t bci = 0; bci < bytecode->length();)
	{
        Instruction insn = bytecode->getInsn(bci);
        if (no_arg(insn))
        {
            if (jump(insn))
            {
                if (insn != BC_JA)
                {
				    int64_t upper = pop_int(); int64_t lower = pop_int();
				    push_int(lower); push_int(upper);
				    switch (insn)
				    {
				    case BC_IFICMPNE:
					    if (upper != lower) bci += bytecode->getInt16(bci + 1) + 1;
					    else bci += insn_len(insn);
					    break;
				    case BC_IFICMPE:
					    if (upper == lower) bci += bytecode->getInt16(bci + 1) + 1;
					    else bci += insn_len(insn);
					    break;
				    case BC_IFICMPG:
					    if (upper > lower) bci += bytecode->getInt16(bci + 1) + 1;
					    else bci += insn_len(insn);
					    break;
				    case BC_IFICMPGE:
					    if (upper >= lower) bci += bytecode->getInt16(bci + 1) + 1;
					    else bci += insn_len(insn);
					    break;
				    case BC_IFICMPL:
					    if (upper < lower) bci += bytecode->getInt16(bci + 1) + 1;
					    else bci += insn_len(insn);
					    break;
				    case BC_IFICMPLE:
					    if (upper <= lower) bci += bytecode->getInt16(bci + 1) + 1;
					    else bci += insn_len(insn);
					    break;
				    default: assert(0);
				    }
                }
                else bci += bytecode->getInt16(bci + 1) + 1;
            }
            else
            {
				switch (insn)
				{
				case BC_DLOAD:
				    push_double(bytecode->getDouble(bci + 1));
					break;
				case BC_ILOAD:
				    push_int(bytecode->getInt64(bci + 1));
					break;
				case BC_SLOAD:
				    push_string(m_code->constantById(bytecode->getUInt16(bci + 1)).c_str());
					break;
				case BC_DLOAD0:
				    push_double(0.0);
					break;
				case BC_ILOAD0:
				    push_int((int64_t)0);
					break;
				case BC_SLOAD0:
				    push_string(m_empty.c_str());
					break;
				case BC_DLOAD1:
				    push_double(1.0);
					break;
				case BC_ILOAD1:
				    push_int((int64_t)1);
					break;
				case BC_DLOADM1:
				    push_double(-1.0);
					break;
				case BC_ILOADM1:
					push_int((int64_t)-1);
					break;
				case BC_LOADDVAR0:
                    push_double(m_double_vars[0]);
					break;
				case BC_LOADDVAR1:
                    push_double(m_double_vars[1]);
					break;
				case BC_LOADDVAR2:
                    push_double(m_double_vars[2]);
					break;
				case BC_LOADDVAR3:
                    push_double(m_double_vars[3]);
					break;
				case BC_LOADIVAR0:
                    push_int(m_int_vars[0]);
					break;
				case BC_LOADIVAR1:
                    push_int(m_int_vars[1]);
					break;
				case BC_LOADIVAR2:
                    push_int(m_int_vars[2]);
					break;
				case BC_LOADIVAR3:
                    push_int(m_int_vars[3]);
					break;
				case BC_LOADSVAR0:
                    push_string(m_string_vars[0]);
					break;
				case BC_LOADSVAR1:
                    push_string(m_string_vars[1]);
					break;
				case BC_LOADSVAR2:
                    push_string(m_string_vars[2]);
					break;
				case BC_LOADSVAR3:
                    push_string(m_string_vars[3]);
					break;
				case BC_LOADDVAR:
                    push_double(load_double(function->id(), bytecode->getUInt16(bci + 1)));
					break;
				case BC_LOADIVAR:
                    push_int(load_int(function->id(), bytecode->getUInt16(bci + 1)));
					break;
				case BC_LOADSVAR:
                    push_string(load_string(function->id(), bytecode->getUInt16(bci + 1)));
					break;
				case BC_LOADCTXDVAR:
                    push_double(load_double(bytecode->getUInt16(bci + 1),
                                            bytecode->getUInt16(bci + 3)));
					break;
				case BC_LOADCTXIVAR:
                    push_int(load_int(bytecode->getUInt16(bci + 1),
                                      bytecode->getUInt16(bci + 3)));
					break;
				case BC_LOADCTXSVAR:
                    push_string(load_string(bytecode->getUInt16(bci + 1),
                                            bytecode->getUInt16(bci + 3)));
					break;
				case BC_CALL:
				{
                    TranslatedFunction *called = m_code->functionById(bytecode->getUInt16(bci + 1));
                    make_call((BytecodeFunction *)called);
					break;
                }
                case BC_CALLNATIVE:
                {
                    Signature const *signature;
                    void const *native = m_code->nativeById(bytecode->getUInt16(bci + 1),
                                                            &signature);
                    assert(native);
                    fill_frame(*signature);
                    switch (return_type(*signature))
                    {
                    case VT_DOUBLE:
                    {
                        double_call dcall = function_cast<double_call>(native);
                        m_double_vars[0] = (*dcall)(m_frame);
                    }
                    break;
                    case VT_STRING:
                    {
                        string_call scall = function_cast<string_call>(native);
                        m_string_vars[0] = (*scall)(m_frame);
                    }
                    break;
                    case VT_INT:
                    {
                        int64_t_call icall = function_cast<int64_t_call>(native);
                        m_int_vars[0] = (*icall)(m_frame);
                    }
                    break;
                    case VT_VOID:
                    {
                        void_call vcall = function_cast<void_call>(native);
                        (*vcall)(m_frame);
                    }
                    break;
                    default: assert(0);
                    }
                }
				case BC_RETURN: return;
				default: assert(0);
				}
                bci += insn_len(insn);
            }
        }
        else if (one_arg(insn))
        {
            value tos = pop();
			switch (insn)
			{
			case BC_DNEG:
			    push_double(-tos._double);
				break;
			case BC_INEG:
			    push_int(-tos._int);
				break;
			case BC_IPRINT:
				print_int(tos._int);
				break;
			case BC_DPRINT:
                print_double(tos._double);
				break;
			case BC_SPRINT:
                print_string(tos._string);
				break;
			case BC_I2D:
			    push_double((double)tos._int);
				break;
			case BC_D2I:
			    push_int((int64_t)tos._double);
				break;
            case BC_S2I:
                push_int((int64_t)tos._string);
                break;
			case BC_POP:
				break;
			case BC_STOREDVAR0:
				m_double_vars[0] = tos._double;
				break;
			case BC_STOREDVAR1:
				m_double_vars[1] = tos._double;
				break;
			case BC_STOREDVAR2:
				m_double_vars[2] = tos._double;
				break;
			case BC_STOREDVAR3:
				m_double_vars[3] = tos._double;
				break;
			case BC_STOREIVAR0:
				m_int_vars[0] = tos._int;
				break;
			case BC_STOREIVAR1:
				m_int_vars[1] = tos._int;
				break;
			case BC_STOREIVAR2:
				m_int_vars[2] = tos._int;
				break;
			case BC_STOREIVAR3:
				m_int_vars[3] = tos._int;
				break;
			case BC_STORESVAR0:
				m_string_vars[0] = tos._string;
				break;
			case BC_STORESVAR1:
				m_string_vars[1] = tos._string;
				break;
			case BC_STORESVAR2:
				m_string_vars[2] = tos._string;
				break;
			case BC_STORESVAR3:
				m_string_vars[3] = tos._string;
				break;
			case BC_STOREDVAR:
			    store_double(function->id(), bytecode->getUInt16(bci + 1), tos._double);
				break;
			case BC_STOREIVAR:
				store_int(function->id(), bytecode->getUInt16(bci + 1), tos._int);
				break;
			case BC_STORESVAR:
				store_string(function->id(), bytecode->getUInt16(bci + 1), tos._string);
				break;
			case BC_STORECTXDVAR:
			    store_double(bytecode->getUInt16(bci + 1),
			                 bytecode->getUInt16(bci + 3),
			                 tos._double);
				break;
			case BC_STORECTXIVAR:
			    store_int(bytecode->getUInt16(bci + 1),
			              bytecode->getUInt16(bci + 3),
			              tos._int);
				break;
			case BC_STORECTXSVAR:
			    store_string(bytecode->getUInt16(bci + 1),
			                 bytecode->getUInt16(bci + 3),
			                 tos._string);
				break;
			default: assert(0);
			}
			bci += insn_len(insn);
        }
        else if (two_arg(insn))
        {
			value upper = pop();
			value lower = pop();
			switch (insn)
			{
			case BC_DADD:
				push_double(upper._double + lower._double);
				break;
			case BC_IADD:
				push_int(upper._int + lower._int);
				break;
			case BC_DSUB:
				push_double(upper._double - lower._double);
				break;
			case BC_ISUB:
				push_int(upper._int - lower._int);
				break;
			case BC_DMUL:
				push_double(upper._double * lower._double);
				break;
			case BC_IMUL:
				push_int(upper._int * lower._int);
				break;
			case BC_DDIV:
				push_double(upper._double / lower._double);
				break;
			case BC_IDIV:
				push_int(upper._int / lower._int);
				break;
			case BC_IMOD:
				push_int(upper._int % lower._int);
				break;
			case BC_SWAP:
				push(upper); push(lower);
				break;
			case BC_DCMP:
			    push_int(dcmp(upper._double, lower._double));
			    break;
			case BC_ICMP:
			    push_int(icmp(upper._int, lower._int));
			    break;
			default: assert(0);
			}
			bci += insn_len(insn);
        }
	}
}

void BCInterpreter::fill_frame(Signature const &signature)
{
    size_t shift = 0;
    for (size_t it = 1; it < signature.size(); ++it)
    {
        switch (signature[it].first)
        {
        case VT_DOUBLE:
            *((double *)(m_frame + shift)) = pop_double();
            shift += sizeof(double);
            break;
        case VT_STRING:
            *((char const **)(m_frame + shift)) = pop_string();
            shift += sizeof(char const *);
            break;
        case VT_INT:
            *((int64_t *)(m_frame + shift)) = pop_int();
            shift += sizeof(int64_t);
            break;
        default: assert(0);
        }
    }
}
