#include "bytecode_interpreter.h"

#include <iostream>

namespace mathvm
{

    Status* bytecode_interpreter::execute(vector < Var* > &vars)
    {
        BytecodeFunction* top = (BytecodeFunction*) functionById(0);
        interpreter_context ctx(top);
        _ctx.push_front(ctx);

        while (!_ctx.empty() && _ctx.front().has_next())
        {
            Instruction insn = _ctx.front().next();

            size_t length;
            const char *name = bytecodeName(insn, &length);
            cout << name << endl;

            execute(insn);
        }

        return Status::Ok();
    }

    void bytecode_interpreter::execute(Instruction insn)
    {
        switch (insn) {
            case BC_DLOAD:
                _stack.push(_ctx.front().real());
                break;
            case BC_ILOAD:
                _stack.push(_ctx.front().int64());
                break;
            case BC_SLOAD:
                _stack.push(_ctx.front().id());
                break;
            case BC_DLOAD0:
                _stack.push((double) 0.0);
                break;
            case BC_ILOAD0:
                _stack.push((int64_t) 0);
                break;
            case BC_SLOAD0:
                _stack.push(makeStringConstant(""));
                break;
            case BC_DLOAD1:
                _stack.push((double) 1.0);
                break;
            case BC_ILOAD1:
                _stack.push((int64_t) 1);
                break;
            case BC_DLOADM1:
                _stack.push((double) -1.0);
                break;
            case BC_ILOADM1:
                _stack.push((int64_t) -1);
                break;
            case BC_DADD:
                _stack.push(load_tos().get_double() + load_tos().get_double());
                break;
            case BC_IADD:
                _stack.push(load_tos().get_int() + load_tos().get_int());
                break;
            case BC_DSUB:
                _stack.push(load_tos().get_double() - load_tos().get_double());
                break;
            case BC_ISUB:
                _stack.push(load_tos().get_int() - load_tos().get_int());
                break;
            case BC_DMUL:
                _stack.push(load_tos().get_double() * load_tos().get_double());
                break;
            case BC_IMUL:
                _stack.push(load_tos().get_int() * load_tos().get_int());
                break;
            case BC_DDIV:
                _stack.push(load_tos().get_double() / load_tos().get_double());
                break;
            case BC_IDIV:
                _stack.push(load_tos().get_int() / load_tos().get_int());
                break;
            case BC_IMOD:
                _stack.push(load_tos().get_int() % load_tos().get_int());
                break;
            case BC_IAOR:
                _stack.push(load_tos().get_int() | load_tos().get_int());
                break;
            case BC_IAAND:
                _stack.push(load_tos().get_int() & load_tos().get_int());
                break;
            case BC_IAXOR:
                _stack.push(load_tos().get_int() ^ load_tos().get_int());
                break;
            case BC_DNEG:
            {
                double value = load_tos().get_double();
                _stack.push(-value);
                break;
            };
            case BC_INEG:
            {
                int64_t value = load_tos().get_int();
                _stack.push(-value);
                break;
            };
            case BC_DPRINT:
                cout << load_tos().get_double();
                break;
            case BC_IPRINT:
                cout << load_tos().get_int();
                break;
            case BC_SPRINT:
                cout << constantById(load_tos().get_id());
                break;
            case BC_I2D:
            {
                int64_t data = load_tos().get_int();
                _stack.push((double) data);
                break;
            };
            case BC_D2I:
            {
                double value = load_tos().get_double();
                _stack.push((int64_t) value);
                break;
            };
            case BC_S2I:
            {
                uint16_t value = load_tos().get_id();
                _stack.push((int64_t) value);
                break;
            };
            case BC_SWAP:
                swap_tos();
                break;
            case BC_POP:
                _stack.pop();
                break;
            case BC_LOADIVAR0:
            case BC_LOADDVAR0:
            case BC_LOADSVAR0:
                _stack.push(_ctx.front().var_by_id(0));
                break;
            case BC_LOADIVAR1:
            case BC_LOADDVAR1:
            case BC_LOADSVAR1:
                _stack.push(_ctx.front().var_by_id(1));
                break;
            case BC_LOADIVAR2:
            case BC_LOADDVAR2:
            case BC_LOADSVAR2:
                _stack.push(_ctx.front().var_by_id(2));
                break;
            case BC_LOADIVAR3:
            case BC_LOADDVAR3:
            case BC_LOADSVAR3:
                _stack.push(_ctx.front().var_by_id(3));
                break;
            case BC_LOADIVAR:
            case BC_LOADDVAR:
            case BC_LOADSVAR:
                _stack.push(_ctx.front().var());
                break;
            case BC_STOREIVAR0:
            case BC_STOREDVAR0:
            case BC_STORESVAR0:
                _ctx.front().store_by_id(load_tos(), 0);
                break;
            case BC_STOREIVAR1:
            case BC_STOREDVAR1:
            case BC_STORESVAR1:
                _ctx.front().store_by_id(load_tos(), 1);
                break;
            case BC_STOREIVAR2:
            case BC_STOREDVAR2:
            case BC_STORESVAR2:
                _ctx.front().store_by_id(load_tos(), 2);
                break;
            case BC_STOREIVAR3:
            case BC_STOREDVAR3:
            case BC_STORESVAR3:
                _ctx.front().store_by_id(load_tos(), 3);
                break;
            case BC_STOREIVAR:
            case BC_STOREDVAR:
            case BC_STORESVAR:
            {
                _ctx.front().store(load_tos());
                break;
            }
            case BC_LOADCTXIVAR:
            case BC_LOADCTXDVAR:
            case BC_LOADCTXSVAR:
                _stack.push(load_context_var());
                break;
            case BC_STORECTXIVAR:
            case BC_STORECTXDVAR:
            case BC_STORECTXSVAR:
            {
                store_context_var(load_tos());
                break;
            }
            case BC_ICMP:
            {
                int64_t first = load_tos().get_int();
                int64_t second = load_tos().get_int();
                if (first < second)
                {
                    _stack.push((int64_t) -1);
                }
                else if (first == second)
                {
                    _stack.push((int64_t) 0);
                }
                else
                {
                    _stack.push((int64_t) 1);
                }
                break;
            };
            case BC_DCMP:
            {
                double first = load_tos().get_double();
                double second = load_tos().get_double();
                if (first < second)
                {
                    _stack.push((int64_t) -1);
                }
                else if (first == second)
                {
                    _stack.push((int64_t) 0);
                }
                else
                {
                    _stack.push((int64_t) 1);
                }
                break;
            };
            case BC_JA:
                _ctx.front().jump(true);
                break;
            case BC_IFICMPNE:
                _ctx.front().jump(load_tos().get_int() != load_tos().get_int());
                break;
            case BC_IFICMPE:
                _ctx.front().jump(load_tos().get_int() == load_tos().get_int());
                break;
            case BC_IFICMPG:
                _ctx.front().jump(load_tos().get_int() > load_tos().get_int());
                break;
            case BC_IFICMPGE:
                _ctx.front().jump(load_tos().get_int() >= load_tos().get_int());
                break;
            case BC_IFICMPL:
                _ctx.front().jump(load_tos().get_int() < load_tos().get_int());
                break;
            case BC_IFICMPLE:
                _ctx.front().jump(load_tos().get_int() <= load_tos().get_int());
                break;
            case BC_STOP:
                break;
            case BC_CALL:
            {
                uint16_t id = _ctx.front().id();
                BytecodeFunction* function = (BytecodeFunction*) functionById(id);

                interpreter_context new_context(function);
                _ctx.push_front(new_context);
                break;
            };
            case BC_CALLNATIVE:
            {

                break;
            }
            case BC_RETURN:
            {
                _ctx.pop_front();
                break;
            };

            default:
                throw ;
        }
    }

} // mathvm
