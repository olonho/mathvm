#include <ast.h>
#include "interpreter_code_impl.h"
#include "bytecode_interpreter_context.h"

namespace mathvm {

using namespace interpreter;

namespace {
const int64_t  ZERO_INT64 = 0;
const int64_t  ONE_INT64 = 1;
}

BytecodeVar InterpreterCodeImpl::pop() {
    auto res = _stack.back();
    _stack.pop_back();
    return res;
}

InterpreterCodeImpl::~InterpreterCodeImpl() {}

InterpreterCodeImpl::InterpreterCodeImpl(ostream &out)
        : _out(out), _status(nullptr) {}

#define NOT_IMPLEMENTED(BC)                                                                \
    case BC:                                                                               \
        _status = Status::Error("invalid bc: " #BC);                                       \
        break;

#define CASE_PRINT(v, BC)                                                                  \
    {                                                                                      \
        auto x = pop();                                                                    \
        _out << x._value.v;                                                                \
        break;                                                                             \
    }

#define UN_OP(op, v)                                                                       \
    {                                                                                      \
        auto x = pop();                                                                    \
        push(op x._value.v);                                                               \
        break;                                                                             \
    }


#define BIN_OP(op, v, BC)                                                                  \
    {                                                                                      \
        auto x = pop();                                                                    \
        auto y = pop();                                                                    \
        push(x._value.v op y._value.v);                                                    \
        break;                                                                             \
    }

#define CASE_CMP_JUMP(op, BC)                       \
    case BC: {                                      \
        auto x = pop()._value.i;                    \
        auto y = pop()._value.i;                    \
        auto offset = reader.read_int16_t();        \
        if (x op y) {                               \
            reader.jump(offset - sizeof(int16_t));  \
        }                                           \
        push(y);                                    \
        push(x);                                    \
        break;                                      \
    }


template <class T>
inline int64_t cmp(T a, T b) {
    return ((a == b) ? ZERO_INT64 : (a > b ? ONE_INT64 : -ONE_INT64));
}

int len(const char*s) {
    int i = 0;
    while (s[i]) {
        ++i;
    }
    return i;
}

Status* InterpreterCodeImpl::execute(vector<Var*>& vars) {
    static std::vector<BytecodeVar> params_cache;
    params_cache.reserve(64);
    _stack.reserve(1 << 20);
    using mathvm::interpreter::pack;
    auto topFunction = static_cast<BytecodeFunction*>(functionByName(AstFunction::top_name));
    uint32_t locals_number = topFunction->localsNumber();

    _contexts.push(BytecodeInterpreterContext(0, 0, topFunction, locals_number));
    _loaded_contexts[0].push(&_contexts.top());

    auto reader = BytecodeReader(topFunction->bytecode());

    while (!reader.empty() && !_status) {
        auto instruction = reader.read_Instruction();
        switch (instruction) {
            case BC_INVALID:
                _status = Status::Error("BC_INVALID in bytecode");
                break;
            case BC_DLOAD:
                push(reader.read_double());
                break;
            case BC_ILOAD:
                push(reader.read_int64_t());
                break;
            case BC_SLOAD:
                push(constantById(reader.read_uint16_t()).c_str());
                break;
            case BC_DLOAD0:
                push(0.0);
                break;
            case BC_ILOAD0:
                push(ZERO_INT64);
                break;
            case BC_SLOAD0:
                push(nullptr);
                break;
            case BC_DLOAD1:
                push(1.0);
                break;
            case BC_ILOAD1:
                push(ONE_INT64);
                break;
            case BC_DLOADM1:
                push(-1.0);
                break;
            case BC_ILOADM1:
                push(-ONE_INT64);
                break;
            case BC_DADD: BIN_OP(+, d, BC_DADD)
            case BC_IADD: BIN_OP(+, i, BC_IADD)
            case BC_DSUB: BIN_OP(-, d, BC_DSUB)
            case BC_ISUB: BIN_OP(-, i, BC_ISUB)
            case BC_DMUL: BIN_OP(*, d, BC_DMUL)
            case BC_IMUL: BIN_OP(*, i, BC_IMUL)
            case BC_DDIV: BIN_OP(/, d, BC_DDIV)
            case BC_IDIV: BIN_OP(/, i, BC_IDIV)
            case BC_IMOD: BIN_OP(%, i, BC_IMOD)
            case BC_DNEG: UN_OP(-, d)
            case BC_INEG: UN_OP(-, i)
            case BC_IAOR: BIN_OP(|, i, BC_IAOR)
            case BC_IAAND: BIN_OP(&, i, BC_IAAND)
            case BC_IAXOR: BIN_OP(^, i, BC_IAXOR)
            case BC_IPRINT:
                CASE_PRINT(i, BC_IPRINT)
            case BC_DPRINT:
                CASE_PRINT(d, BC_IPRINT)
            case BC_SPRINT:
                CASE_PRINT(s, BC_IPRINT)
            case BC_I2D: {
                double d = pop()._value.i;
                push(d);
                break;
            }
            case BC_D2I: {
                int64_t  i = (int64_t) pop()._value.d;
                push(i);
                break;
            }
            case BC_S2I: {
                int64_t i = (int64_t) pop()._value.s;
                push(i);
                break;
            }
            case BC_SWAP: {
                auto x = pop();
                auto y = pop();
                push(x);
                push(y);
                break;
            }
            case BC_POP: {
                pop();
                break;
            }
            case BC_LOADDVAR:
            case BC_LOADIVAR:
            case BC_LOADSVAR:
                load_var_local(reader);
                break;
            case BC_LOADDVAR0:
            case BC_LOADIVAR0:
            case BC_LOADSVAR0:
                load_var_cached(reader, 0);
                break;
            case BC_LOADDVAR1:
            case BC_LOADIVAR1:
            case BC_LOADSVAR1:
                load_var_cached(reader, 1);
                break;
            case BC_LOADDVAR2:
            case BC_LOADIVAR2:
            case BC_LOADSVAR2:
                load_var_cached(reader, 2);
                break;
            case BC_LOADDVAR3:
            case BC_LOADIVAR3:
            case BC_LOADSVAR3:
                load_var_cached(reader, 3);
                break;
            case BC_LOADCTXDVAR:
            case BC_LOADCTXIVAR:
            case BC_LOADCTXSVAR:
                load_var_ctx(reader);
                break;

            case BC_STOREDVAR:
            case BC_STOREIVAR:
            case BC_STORESVAR:
                store_var_local(reader);
                break;
            case BC_STOREDVAR0:
            case BC_STOREIVAR0:
            case BC_STORESVAR0:
                store_var_cached(0);
                break;
            case BC_STOREDVAR1:
            case BC_STOREIVAR1:
            case BC_STORESVAR1:
                store_var_cached(1);
                break;
            case BC_STOREDVAR2:
            case BC_STOREIVAR2:
            case BC_STORESVAR2:
                store_var_cached(2);
                break;
            case BC_STOREDVAR3:
            case BC_STOREIVAR3:
            case BC_STORESVAR3:
                store_var_cached(3);
                break;
            case BC_STORECTXDVAR:
            case BC_STORECTXIVAR:
            case BC_STORECTXSVAR:
                store_var_ctx(reader);
                break;

            case BC_DCMP: {
                auto x = pop();
                auto y = pop();
                push(cmp(x._value.d, y._value.d));
                break;
            }
            case BC_ICMP:{
                auto x = pop();
                auto y = pop();
                push(cmp(x._value.i, y._value.i));
                break;
            }
            case BC_JA: {
                auto offset = reader.read_int16_t();
                reader.jump(offset - sizeof offset);
                break;
            }
            CASE_CMP_JUMP(!=, BC_IFICMPNE)
            CASE_CMP_JUMP(==, BC_IFICMPE)
            CASE_CMP_JUMP(>, BC_IFICMPG)
            CASE_CMP_JUMP(>=, BC_IFICMPGE)
            CASE_CMP_JUMP(<, BC_IFICMPL)
            CASE_CMP_JUMP(<=, BC_IFICMPLE)
            case BC_CALL: {
                auto id = reader.read_uint16_t();
                auto func = dynamic_cast<BytecodeFunction*>(functionById(id));

                auto n = func->parametersNumber();
                auto& params = params_cache;
                if (n > params.size()) {
                    params.resize(n);
                }
                for (int32_t i = 0; i < n; ++i) {
                    params[i] = pop();
                }

                _contexts.push(
                        BytecodeInterpreterContext(reader.index(), (uint32_t) _stack.size(), func, func->localsNumber())
                );
                _loaded_contexts[id].push(&_contexts.top());

                for (uint16_t i = 0; i < func->parametersNumber(); ++i) {
                    _contexts.top().set_local_value(i, params[i]);
                }
                reader = BytecodeReader(_contexts.top().get_bytecode());
                break;
            }
            case BC_CALLNATIVE: {
                auto id = reader.read_uint16_t();
                const std::string* name;
                const Signature* signature;
                auto native_ptr = (void*)nativeById(id, &signature, &name);
                call_native(signature, native_ptr);
                break;
            }
            case BC_RETURN: {
                if (_contexts.size() == 1) {
                    _out.flush();
                    return Status::Ok();
                }

                auto& context = _contexts.top();
                auto context_id = context.id();
                auto return_type = context.get_return_type();

                bool has_return = (return_type != VT_INVALID && return_type != VT_VOID);
                BytecodeVar return_value;
                if (has_return) {
                    return_value = pop();
                }

                while (_stack.size() != context.get_stackframe()) {
                    pop();
                }
                if (has_return) {
                    push(return_value);
                }
                auto call_address = context.get_call_address();
                _loaded_contexts[context_id].pop();
                _contexts.pop();
                reader = BytecodeReader(_contexts.top().get_bytecode(), call_address);
                break;
            }
            NOT_IMPLEMENTED(BC_DUMP)
            NOT_IMPLEMENTED(BC_STOP)
            NOT_IMPLEMENTED(BC_BREAK)
            NOT_IMPLEMENTED(BC_LAST)
            default:
                _status = Status::Error("Unknown instruction");
        }
    }

    _out.flush();

    return !reader.empty() ? Status::Warning("Instructions ended before BC_RETURN") : Status::Ok();
}

void InterpreterCodeImpl::load_var_ctx(BytecodeReader& reader) {
    auto context_id = reader.read_uint16_t();
    auto var_id = reader.read_uint16_t();
    push(_loaded_contexts[context_id].top()->get_local_value(var_id));
}

void InterpreterCodeImpl::load_var_local(BytecodeReader &reader) {
    auto id = reader.read_uint16_t();
    push(_contexts.top().get_local_value(id));
}

void InterpreterCodeImpl::load_var_cached(BytecodeReader &reader, uint16_t id) {
    push(_contexts.top().get_local_value(id));
}

void InterpreterCodeImpl::store_var_ctx(BytecodeReader &reader) {
    auto context_id = reader.read_uint16_t();
    auto var_id = reader.read_uint16_t();
    _loaded_contexts[context_id].top()->set_local_value(var_id, pop());
}

void InterpreterCodeImpl::store_var_local(BytecodeReader &reader) {
    auto id = reader.read_uint16_t();
    _contexts.top().set_local_value(id, pop());
}

void InterpreterCodeImpl::store_var_cached(uint16_t id) {
    _contexts.top().set_local_value(id, pop());
}

#define RETURN_SWITCH(ARG_TYPES, ARGS) \
switch (return_type) { \
    case VT_INVALID:break;\
    case VT_VOID:          \
        (void) reinterpret_cast<void (*)ARG_TYPES>(func)ARGS;\
    break;                                \
    case VT_DOUBLE:                        \
            push(reinterpret_cast<double (*)ARG_TYPES>(func)ARGS);\
    break;                                                \
    case VT_INT:                                           \
            push(reinterpret_cast<int64_t (*)ARG_TYPES>(func)ARGS);  \
    break;                                                   \
    case VT_STRING:                                           \
            push(reinterpret_cast<const char* (*)ARG_TYPES>(func)ARGS); \
    break;                                                      \
}

void InterpreterCodeImpl::call_native(const Signature* signature, void* func) {
    size_t n = signature->size() - 1;
    auto return_type = signature->at(0).first;
    if (n == 0) {
        RETURN_SWITCH((), ())
    } else if (n == 1) {
        auto arg1_t = signature->at(1).first;
        auto arg1 = pop();
        switch (arg1_t) {
            case VT_DOUBLE: {
                RETURN_SWITCH((double), (arg1._value.d))
                break;
            }
            case VT_INT:
                RETURN_SWITCH((int64_t) , (arg1._value.i))
                break;
            case VT_STRING:
                RETURN_SWITCH((const char*) , (arg1._value.s))
                break;
            default:break;
        }
    } else if (n == 2) {
        auto arg2_t = signature->at(2).first;
        auto arg2 = pop();
        auto arg1_t = signature->at(1).first;
        auto arg1 = pop();

        switch (arg1_t) {
            case VT_DOUBLE: {
                switch (arg2_t){
                    case VT_DOUBLE:
                        RETURN_SWITCH((double, double), (arg1._value.d, arg2._value.d))
                        break;
                    case VT_INT:
                        RETURN_SWITCH((double, int64_t), (arg1._value.d, arg2._value.i))
                        break;
                    case VT_STRING:
                        RETURN_SWITCH((double, const char*), (arg1._value.d, arg2._value.s))
                        break;
                    default:break;
                }
                break;
            }
            case VT_INT:
                switch (arg2_t){
                    case VT_DOUBLE:
                        RETURN_SWITCH((int64_t, double), (arg1._value.i, arg2._value.d))
                        break;
                    case VT_INT:
                        RETURN_SWITCH((int64_t, int64_t), (arg1._value.i, arg2._value.i))
                        break;
                    case VT_STRING:
                        RETURN_SWITCH((int64_t, const char*), (arg1._value.i, arg2._value.s))
                        break;
                    default:break;
                }
                break;
            case VT_STRING:
                switch (arg2_t){
                    case VT_DOUBLE:
                        RETURN_SWITCH((const char*, double), (arg1._value.s, arg2._value.d))
                        break;
                    case VT_INT:
                        RETURN_SWITCH((const char*, int64_t), (arg1._value.s, arg2._value.i))
                        break;
                    case VT_STRING:
                        RETURN_SWITCH((const char*, const char*), (arg1._value.s, arg2._value.s))
                        break;
                    default:break;
                }
                break;
            default:break;
        }
    } else {
        _status = Status::Error("Cannot call native functions with more than 2 arguments");
        // ╭∩╮(-_-)╭∩╮
    }

}

void InterpreterCodeImpl::push(BytecodeVar value)  {
    _stack.push_back(value);
}

} // namespace mathvm