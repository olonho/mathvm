#include "bytecode_interpreter.h"

using namespace mathvm;

namespace
{
    int16_t instruction_length[] = {
    #define INSN_LEN(b, d, l) l,
            FOR_BYTECODES(INSN_LEN)
    #undef INSN_LEN
            0
    };

    std::string instruction_name[] = {
#define INSN_NAME(b, d, l) #b,
            FOR_BYTECODES(INSN_NAME)
#undef INSN_NAME
            ""
    };

    #define DO_INT_BINOP(OP) \
    int64_t a = pop_value<int64_t>(); \
    int64_t b = pop_value<int64_t>(); \
    push_value<int64_t>(a OP b);

    #define DO_DOUBLE_BINOP(OP) \
    double a = pop_value<double>(); \
    double b = pop_value<double>(); \
    push_value<double>(a OP b);

    #define DO_CMP_JMP(OP) \
    int64_t a = pop_value<int64_t>(); \
    int64_t b = pop_value<int64_t>(); \
    int16_t jump_off = bc().getInt16(ip + 1); \
    if (a OP b) { \
        ip_skip = 0; \
        ip += jump_off + 1; \
    }
}

Status* BytecodeInterpreter::run() {
    try {
        interpret();
    } catch (...) {
        return Status::Error("=(");
    }
    return Status::Ok();
}

Bytecode& BytecodeInterpreter::bc() {
    return *_bytecode_funs.back()->bytecode();
}


void BytecodeInterpreter::interpret() {
    uint32_t ip = 0;
    while (true) {
        auto instruction = bc().getInsn(ip);
//        std::cout << "Executing: " << instruction_name[instruction]
//                  << " ; in function = " << cur_fun_id()
//                  << " ; cur context = " << cur_fun_ctx()
//                  << std::endl;
        auto ip_skip = instruction_length[instruction];
        switch (instruction) {
            case BC_STOP: {
                return;
            }
            case BC_LAST:
            case BC_INVALID:
                throw InterpreterError("bad instruction (invalid or last");
            case BC_DLOAD: {
                push_value<double>(bc().getDouble(ip + 1));
                break;
            }
            case BC_ILOAD: {
                push_value<int64_t>(bc().getInt64(ip + 1));
                break;
            }
            case BC_SLOAD: {
                auto id = bc().getUInt16(ip + 1);
                push_value(_code->constantById(id).c_str());
                break;
            }
            case BC_DLOAD0: {
                push_value<double>(0.0);
                break;
            }
            case BC_ILOAD0: {
                push_value<int64_t>(0);
                break;
            }
            case BC_SLOAD0: {
                push_value<const char*>(0);
                break;
            }
            case BC_DLOAD1: {
                push_value<double>(1);
                break;
            }
            case BC_ILOAD1: {
                push_value<int64_t>(1);
                break;
            }
            case BC_DLOADM1: {
                push_value<double>(-1);
                break;
            }
            case BC_ILOADM1: {
                push_value<int64_t>(1);
                break;
            }
            case BC_IADD: {
                DO_INT_BINOP(+)
                break;
            }
            case BC_ISUB: {
                DO_INT_BINOP(-)
                break;
            }
            case BC_IMUL: {
                DO_INT_BINOP(*)
                break;
            }
            case BC_IDIV: {
                DO_INT_BINOP(/)
                break;
            }
            case BC_IMOD: {
                DO_INT_BINOP(%)
                break;
            }
            case BC_IAOR: {
                DO_INT_BINOP(|)
                break;
            }
            case BC_IAAND: {
                DO_INT_BINOP(&)
                break;
            }
            case BC_IAXOR: {
                DO_INT_BINOP(^)
                break;
            }
            case BC_DADD: {
                DO_DOUBLE_BINOP(+)
                break;
            }
            case BC_DSUB: {
                DO_DOUBLE_BINOP(-)
                break;
            }
            case BC_DMUL: {
                DO_DOUBLE_BINOP(*)
                break;
            }
            case BC_DDIV: {
                DO_DOUBLE_BINOP(/)
                break;
            }
            case BC_INEG: {
                push_value(-pop_value<int64_t>());
                break;
            }
            case BC_DNEG: {
                push_value(-pop_value<double>());
                break;
            }
            case BC_IPRINT: {
                _out << pop_value<int64_t>();
                break;
            }
            case BC_DPRINT: {
                _out << pop_value<double>();
                break;
            }
            case BC_SPRINT: {
                _out << pop_value<const char*>();
                break;
            }
            case BC_I2D: {
                push_value<double>(pop_value<int64_t>());
                break;
            }
            case BC_D2I: {
                push_value(static_cast<int64_t>(pop_value<double>()));
                break;
            }
            case BC_S2I: {
                push_value(reinterpret_cast<int64_t>(pop_value<const char *>()));
                break;
            }
            case BC_SWAP: {
                auto a = pop_value<StackVal>();
                auto b = pop_value<StackVal>();
                push_value(a);
                push_value(b);
                break;
            }
            case BC_POP: {
                _stack.pop_back();
                break;
            }
            case BC_LOADDVAR0:
            case BC_LOADIVAR0:
            case BC_LOADSVAR0: {
                push_local(0);
                break;
            }
            case BC_LOADDVAR1:
            case BC_LOADIVAR1:
            case BC_LOADSVAR1: {
                push_local(1);
                break;
            }
            case BC_LOADDVAR2:
            case BC_LOADIVAR2:
            case BC_LOADSVAR2: {
                push_local(2);
                break;
            }
            case BC_LOADDVAR3:
            case BC_LOADIVAR3:
            case BC_LOADSVAR3: {
                push_local(3);
                break;
            }
            case BC_STOREDVAR0:
            case BC_STOREIVAR0:
            case BC_STORESVAR0: {
                store_local(0);
                break;
            }
            case BC_STOREDVAR1:
            case BC_STOREIVAR1:
            case BC_STORESVAR1: {
                store_local(1);
                break;
            }
            case BC_STOREDVAR2:
            case BC_STOREIVAR2:
            case BC_STORESVAR2: {
                store_local(2);
                break;
            }
            case BC_STOREDVAR3:
            case BC_STOREIVAR3:
            case BC_STORESVAR3: {
                store_local(3);
                break;
            }
            case BC_LOADDVAR:
            case BC_LOADIVAR:
            case BC_LOADSVAR: {
                auto id = bc().getUInt16(ip + 1);
                push_local(id);
                break;
            }
            case BC_STOREDVAR:
            case BC_STOREIVAR:
            case BC_STORESVAR: {
                auto id = bc().getUInt16(ip + 1);
                store_local(id);
                break;
            }
            case BC_LOADCTXDVAR:
            case BC_LOADCTXIVAR:
            case BC_LOADCTXSVAR: {
                auto ctx = bc().getUInt16(ip + 1);
                auto id = bc().getUInt16(ip + 3);
                push_from_context(ctx, id);
                break;
            }
            case BC_STORECTXDVAR:
            case BC_STORECTXIVAR:
            case BC_STORECTXSVAR: {
                auto ctx = bc().getUInt16(ip + 1);
                auto id = bc().getUInt16(ip + 3);
                store_to_context(ctx, id);
                break;
            }
            case BC_DCMP: {
                auto a = pop_value<double>();
                auto b = pop_value<double>();
                push_value<int64_t>((a == b) ? 0 : ((a < b) ? -1 : 1));
                break;
            }
            case BC_ICMP: {
                auto a = pop_value<int64_t>();
                auto b = pop_value<int64_t>();
                push_value<int64_t>((a == b) ? 0 : ((a < b) ? -1 : 1));
                break;
            }
            case BC_JA: {
                ip += 1 + bc().getInt16(ip + 1);
                ip_skip = 0;
                break;
            }
            case BC_IFICMPNE: {
                DO_CMP_JMP(!=)
                break;
            }
            case BC_IFICMPE: {
                DO_CMP_JMP(==)
                break;
            }
            case BC_IFICMPG: {
                DO_CMP_JMP(>)
                break;
            }
            case BC_IFICMPGE: {
                DO_CMP_JMP(>=)
                break;
            }
            case BC_IFICMPL: {
                DO_CMP_JMP(<)
                break;
            }
            case BC_IFICMPLE: {
                DO_CMP_JMP(<=)
                break;
            }
            case BC_DUMP: {
                push_value(_stack.back());
                break;
            }
            case BC_CALL: {
                auto fun_id = bc().getUInt16(ip + 1);
                auto bf = dynamic_cast<BytecodeFunction*>(_code->functionById(fun_id));
                push_fun(bf);
                _call_stack.push_back(ip + ip_skip);
                ip = 0;
                ip_skip = 0;
                break;
            }
            case BC_CALLNATIVE: {
                // TODO
                break;
            }
            case BC_RETURN: {
                pop_fun();
                ip = _call_stack.back();
                _call_stack.pop_back();
                ip_skip = 0;
                break;
            }
            case BC_BREAK: // ?
                break;
        }
        ip += ip_skip;
    }
}
