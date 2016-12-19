#include <dlfcn.h>
#include "vm.h"
#include <unordered_map>


namespace mathvm {

#define DBG_ADDR(label, addr) \
cout << endl << label << " 0x" << hex << reinterpret_cast<uint64_t>(addr) << endl;

#define D_BINOP(op) \
double left = stack_.back().double_value; stack_.pop_back(); \
double right = stack_.back().double_value; stack_.pop_back(); \
stack_.push_back(StackUnit(static_cast<double>(left op right)));

#define I_BINOP(op) \
int64_t left = stack_.back().int_value; stack_.pop_back(); \
int64_t right = stack_.back().int_value; stack_.pop_back(); \
stack_.push_back(StackUnit(static_cast<int64_t>(left op right)));

#define v64(val) StackUnit(static_cast<int64_t>(val))

#define READPOP(type) \
stack_.back().type; stack_.pop_back()

#define GETLOCAL(id) \
locals_.top().locals[id]

#define STORELOCAL(id) \
locals_.top().locals[id] = stack_.back(); \
stack_.pop_back();

#define CCMP(a, b) ((a == b) ? 0L : (a > b ? 1L : -1L))

#define IIFCMP(sign) \
int64_t left = READPOP(int_value); \
int64_t right = READPOP(int_value); \
int16_t label = bytecode().getInt16(ip + 1); \
if (left sign right) { \
    ip_offset = 0; \
    ip += 1 + label; \
}

static const struct {
        const char *name;
        Instruction insn;
        uint8_t length;
    } names[] = {
#define BC_NAME(b, d, l) {#b, BC_##b, l},
            FOR_BYTECODES(BC_NAME)};

vm::vm(mathvm::Code &code, ostream &output)
        : code_(code), output_(output) {
    stack_.reserve(STACK_SIZE);

    auto iter = Code::FunctionIterator(&code);

    uint16_t functions_count = 0;
    while (iter.hasNext()) {
        iter.next();
        functions_count += 1;
    }

    contexts_.resize(functions_count);
}

int vm::run() {
    function_stack_.push(dynamic_cast<BytecodeFunction *>(code_.functionById(0)));
    uint32_t locals_number = function_stack_.top()->localsNumber();
    locals_.push(details::Context(locals_number));
    contexts_[0].push(&locals_.top());
    call_stack_.push(0);

    return repl();
}

int vm::repl() {
    using namespace details;
    uint32_t ip = 0;
    while (true) {
        Instruction instruction = bytecode().getInsn(ip);
        int32_t ip_offset = names[instruction].length;
        switch (instruction) {
            case BC_DLOAD:
                stack_.push_back(bytecode().getDouble(ip + 1));
                break;
            case BC_ILOAD:
                stack_.push_back(bytecode().getInt64(ip + 1));
                break;
            case BC_SLOAD:
                stack_.push_back(
                        reinterpret_cast<int64_t>(code_.constantById(bytecode().getUInt16(ip + 1)).c_str()));
                break;
            case BC_DLOAD0:
                stack_.push_back(0.0);
                break;
            case BC_ILOAD0:
            case BC_SLOAD0:
                stack_.push_back(v64(0));
                break;
            case BC_DLOAD1:
                stack_.push_back(1.0);
                break;
            case BC_ILOAD1:
                stack_.push_back(v64(1));
                break;
            case BC_DLOADM1:
                stack_.push_back(-1.0);
                break;
            case BC_ILOADM1:
                stack_.push_back(v64(-1));
                break;
            case BC_DADD: {
                D_BINOP(+)
                break;
            }
            case BC_IADD: {
                I_BINOP(+)
                break;
            }
            case BC_DSUB: {
                D_BINOP(-)
                break;
            }
            case BC_ISUB: {
                I_BINOP(-)
                break;
            }
            case BC_DMUL: {
                D_BINOP(*)
                break;
            }
            case BC_IMUL: {
                I_BINOP(*)
                break;
            }
            case BC_DDIV: {
                D_BINOP(/)
                break;
            }
            case BC_IDIV: {
                I_BINOP(/)
                break;
            }
            case BC_IMOD: {
                I_BINOP(%)
                break;
            }
            case BC_DNEG:
                stack_.at(stack_.size() - 1).double_value *= -1.0;
                break;
            case BC_INEG:
                stack_.at(stack_.size() - 1).int_value *= -1;
                break;
            case BC_IAOR: {
                I_BINOP(|)
                break;
            }
            case BC_IAAND: {
                I_BINOP(&)
                break;
            }
            case BC_IAXOR: {
                I_BINOP(^)
                break;
            }
            case BC_IPRINT: {
                int64_t value = READPOP(int_value);
                output_ << value;
                break;
            }
            case BC_DPRINT: {
                double value = READPOP(double_value);
                output_ << value;
                break;
            }
            case BC_SPRINT: {
                int64_t value = READPOP(int_value);
                char const *str = reinterpret_cast<char const *>(value);
                output_ << str;
                break;
            }
            case BC_I2D: {
                double i = READPOP(int_value);
                stack_.push_back(i);
                break;
            }
            case BC_D2I: {
                double value = READPOP(double_value);
                int64_t result = static_cast<int64_t>(value);
                stack_.push_back(v64(result));
                break;
            }
            case BC_S2I: {
                int64_t result = READPOP(int_value);
                stack_.push_back(v64(result));
                break;
            }
            case BC_SWAP: {
                auto left = READPOP(int_value);
                auto right = READPOP(int_value);
                stack_.push_back(left);
                stack_.push_back(right);
                break;
            }
            case BC_POP:
                stack_.pop_back();
                break;
            case BC_LOADDVAR0:
            case BC_LOADSVAR0:
            case BC_LOADIVAR0:
                stack_.push_back(GETLOCAL(0));
                break;
            case BC_LOADDVAR1:
            case BC_LOADIVAR1:
            case BC_LOADSVAR1:
                stack_.push_back(GETLOCAL(1));
                break;
            case BC_LOADDVAR2:
            case BC_LOADIVAR2:
            case BC_LOADSVAR2:
                stack_.push_back(GETLOCAL(2));
                break;
            case BC_LOADDVAR3:
            case BC_LOADIVAR3:
            case BC_LOADSVAR3:
                stack_.push_back(GETLOCAL(3));
                break;
            case BC_STOREDVAR0:
            case BC_STOREIVAR0:
            case BC_STORESVAR0:
            STORELOCAL(0)
                break;
            case BC_STOREDVAR1:
            case BC_STOREIVAR1:
            case BC_STORESVAR1:
            STORELOCAL(1)
                break;
            case BC_STOREDVAR2:
            case BC_STOREIVAR2:
            case BC_STORESVAR2:
            STORELOCAL(2)
                break;
            case BC_STOREDVAR3:
            case BC_STOREIVAR3:
            case BC_STORESVAR3:
            STORELOCAL(3)
                break;
            case BC_LOADSVAR:
            case BC_LOADIVAR:
            case BC_LOADDVAR: {
                uint16_t id = bytecode().getUInt16(ip + 1);
                stack_.push_back(locals_.top().locals[id]);
                break;
            }
            case BC_STORESVAR:
            case BC_STOREIVAR:
            case BC_STOREDVAR: {
                uint16_t id = bytecode().getUInt16(ip + 1);
                locals_.top().locals[id] = stack_.back();
                stack_.pop_back();
                break;
            }
            case BC_LOADCTXSVAR:
            case BC_LOADCTXIVAR:
            case BC_LOADCTXDVAR: {
                uint16_t ctx = bytecode().getUInt16(ip + 1);
                uint16_t id = bytecode().getUInt16(ip + 3);
                stack_.push_back(contexts_[ctx].top()->locals[id]);
                break;
            }
            case BC_STORECTXSVAR:
            case BC_STORECTXIVAR:
            case BC_STORECTXDVAR: {
                uint16_t ctx = bytecode().getUInt16(ip + 1);
                uint16_t id = bytecode().getUInt16(ip + 3);
                contexts_[ctx].top()->locals[id] = stack_.back();
                stack_.pop_back();
                break;
            }
            case BC_DCMP: {
                double left = READPOP(double_value);
                double right = READPOP(double_value);
                int64_t result = CCMP(left, right);
                stack_.push_back(result);
                break;
            }
            case BC_ICMP: {
                int64_t left = READPOP(int_value);
                int64_t right = READPOP(int_value);
                int64_t result = CCMP(left, right);
                stack_.push_back(result);
                break;
            }
            case BC_JA: {
                ip += 1 + bytecode().getInt16(ip + 1);
                ip_offset = 0;
                break;
            }
            case BC_IFICMPNE: {
                IIFCMP(!=)
                break;
            }
            case BC_IFICMPE: {
                IIFCMP(==)
                break;
            }
            case BC_IFICMPG: {
                IIFCMP(>)
                break;
            }
            case BC_IFICMPGE: {
                IIFCMP(>=)
                break;
            }
            case BC_IFICMPL: {
                IIFCMP(<)
                break;
            }
            case BC_IFICMPLE: {
                IIFCMP(<=)
                break;
            }
            case BC_DUMP: {
                stack_.push_back(stack_.back());
                break;
            }
            case BC_STOP: {
                output_ << endl << "Execution stopeed" << endl;
                exit(0);
            }
            case BC_CALL: {
                uint16_t id = bytecode().getUInt16(ip + 1);
                BytecodeFunction *function = dynamic_cast<BytecodeFunction *>(code_.functionById(id));

                locals_.push(function->localsNumber());
                contexts_[id].push(&locals_.top());

                for (int32_t i = function->parametersNumber() - 1; i >= 0; --i) {
                    STORELOCAL(i)
                }

                stack_frames_.push(stack_.size());
                call_stack_.push(ip + names[instruction].length);
                ip = 0;
                ip_offset = 0;

                function_stack_.push(function);
                break;
            }
            case BC_CALLNATIVE: {
                uint16_t id = bytecode().getUInt16(ip + 1);

                string const* name;
                Signature const* signature;
                native_handler handler = reinterpret_cast<native_handler>(code_.nativeById(id, &signature, &name));

                size_t args_count = signature->size() - 1;
                StackUnit *args_ptr = 0;
                if (args_count > 0) {
                    args_ptr = &stack_[stack_.size() - args_count];
                }

                void const *result = handler(args_ptr);
                for (size_t i = 0; i < args_count; ++i) {
                    stack_.pop_back();
                }

                if (signature->at(0).first != VT_VOID) {
                    stack_.push_back(StackUnit(reinterpret_cast<int64_t>(result)));
                }
                break;
            }
            case BC_RETURN: {
                if (function_stack_.size() == 1) {
                    return 0;
                }

                uint16_t current_id = function_stack_.top()->id();
                function_stack_.pop();

                StackUnit returnValue = stack_.back();
                while (stack_.size() != stack_frames_.top()) {
                    stack_.pop_back();
                }
                stack_frames_.pop();
                stack_.push_back(returnValue);
                locals_.pop();

                contexts_[current_id].pop();
                ip = call_stack_.top();
                call_stack_.pop();
                ip_offset = 0;
                break;
            }
            case BC_LAST:
            case BC_INVALID:
                return -1;
            default:
                break;
        }

        ip += ip_offset;
    }
}

Bytecode &vm::bytecode() {
    return *function_stack_.top()->bytecode();
}

}
