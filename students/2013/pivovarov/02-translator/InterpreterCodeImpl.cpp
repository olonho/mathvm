#include "InterpreterCodeImpl.h"

namespace mathvm {

#define STACK_SIZE 1024 * 1024

Status * InterpreterCodeImpl::execute(vector<Var*> & vars) {
    return new Status("NOT IMPLEMENTED");
}

void InterpreterCodeImpl::addFunctionData(uint16_t id, FunctionData data) {
    if (funsData.size() <= id) {
        funsData.resize(id + 1);
    }
    funsData[id] = data;
}

FunctionData const & InterpreterCodeImpl::getFunctionData(uint16_t id) {
    return funsData[id];
}

union Data {
    Data() {}
    Data(int64_t i) : i(i) {}
    Data(double d) : d(d) {}
    Data(char * s) : s(s) {}

    int64_t i;
    double d;
    char * s;
};

struct CallData {
    CallData() {}
    CallData(uint16_t id, uint32_t last_stack, uint16_t stack_size)
        : id(id), last_stack(last_stack), stack_size(stack_size), index(0) {}

    uint16_t id;
    uint32_t last_stack;
    uint16_t stack_size;
    uint32_t index;
};

template<class T>
inline T pop(vector<T> & v) {
    T tmp = v.back();
    v.pop_back();
    return tmp;
}

#define INC(x, inc) (((x)+=(inc))-(inc))
#define INC_8(x) INC(x, 1)
#define INC_16(x) INC(x, 2)
#define INC_32(x) INC(x, 4)
#define INC_64(x) INC(x, 8)

#define GET_DATA_8()                        \
    *(uint8_t*)(data + INC_8(index))
#define GET_DATA_16()                       \
    *(uint16_t*)(data + INC_16(index))
#define GET_DATA_32()                       \
    *(uint32_t*)(data + INC_32(index))
#define GET_DATA_64()                       \
    *(int64_t*)(data + INC_64(index))
#define GET_DATA()                          \
    GET_DATA_64()

#define PEEK_DATA_S16()                     \
    *(int16_t*)(data + index)

#define GET_INSN()                          \
    *(uint8_t*)(data + INC_8(index))

#define PUSH(V)                             \
    stack[++stack_top] = V
#define PUSH_D(V)                           \
    PUSH((double)V)
#define PUSH_I(V)                           \
    PUSH((int64_t)V)
#define PUSH_S(V)                           \
    PUSH((char *)V)

#define POP()                               \
    stack[stack_top--]
#define TOP()                               \
    stack[stack_top]

#define CHECK_STACK_OF( N )                 \
    if (stack_top + N >= STACK_SIZE) {     \
      return new Status("Stack overflow");  \
    }

#define VAR(CONTEXT, VARIABLE)              \
    context_stack[fun_context[CONTEXT] + VARIABLE]

#define NEXT                                \
    goto *(labels[GET_INSN()])

#define RETURN_ERR( STRING )                \
    {                                       \
        delete[] stack;                     \
        return new Status(STRING);          \
    }

#define RETURN()                            \
    {                                       \
        delete[] stack;                     \
        return new Status();                \
    }

Status * InterpreterCodeImpl::execute() {
    uint32_t index;
    uint8_t * data;
    uint32_t stack_top;

    vector<CallData> call_stack;
    vector<uint32_t> fun_context;
    vector<Data> context_stack;
    Data * stack = new Data[STACK_SIZE];

    fun_context.resize(funsData.size());
    call_stack.reserve(1024);
    context_stack.reserve(4096);

    void * labels[85] = {
    #define LABEL_POINTER(b, d, l) &&b,
        FOR_BYTECODES(LABEL_POINTER)
    #undef LABEL_POINTER
    NULL };

    Data v1;
    Data v2;

    uint16_t context_p;
    uint16_t variable_p;
    FunctionData fun_data;
    CallData call_data;
    uint16_t fun_id;

    shared_ptr<NativeFunction_> native_fun;
    int64_t intArgs[6];
    double doubleArgs[8];
    size_t intIdx = 0;
    size_t doubleIdx = 0;

    stack_top = 0;
    fun_data = getFunctionData(0);
    call_stack.push_back(CallData(0, fun_context[0], fun_data.stack_size));
    context_stack.resize(fun_data.stack_size);
    data = fun_data.fun->bytecode()->getData();
    index = 0;
    NEXT;

    INVALID:
        RETURN_ERR("BC_INVALID");
    DLOAD:
        CHECK_STACK_OF(1);
        PUSH( GET_DATA_64() );
        NEXT;
    ILOAD:
        CHECK_STACK_OF(1);
        PUSH( GET_DATA_64() );
        NEXT;
    SLOAD:
        CHECK_STACK_OF(1);
        PUSH_S( constantById(GET_DATA_16()).c_str() );
        NEXT;
    DLOAD0:
        CHECK_STACK_OF(1);
        PUSH_D(0);
        NEXT;
    ILOAD0:
        CHECK_STACK_OF(1);
        PUSH_I(0);
        NEXT;
    SLOAD0:
        CHECK_STACK_OF(1);
        PUSH_S( constantById(0).c_str() );
        NEXT;
    DLOAD1:
        CHECK_STACK_OF(1);
        PUSH_D(1);
        NEXT;
    ILOAD1:
        CHECK_STACK_OF(1);
        PUSH_I(1);
        NEXT;
    DLOADM1:
        CHECK_STACK_OF(1);
        PUSH_D(-1);
        NEXT;
    ILOADM1:
        CHECK_STACK_OF(1);
        PUSH_I(-1);
        NEXT;
    DADD:
        v1 = POP();
        v2 = POP();
        PUSH_D(v1.d + v2.d);
        NEXT;
    IADD:
        v1 = POP();
        v2 = POP();
        PUSH_I(v1.i + v2.i);
        NEXT;
    DSUB:
        v1 = POP();
        v2 = POP();
        PUSH_D(v1.d - v2.d);
        NEXT;
    ISUB:
        v1 = POP();
        v2 = POP();
        PUSH_I(v1.i - v2.i);
        NEXT;
    DMUL:
        v1 = POP();
        v2 = POP();
        PUSH_D(v1.d * v2.d);
        NEXT;
    IMUL:
        v1 = POP();
        v2 = POP();
        PUSH_I(v1.i * v2.i);
        NEXT;
    DDIV:
        v1 = POP();
        v2 = POP();
        PUSH_D(v1.d / v2.d);
        NEXT;
    IDIV:
        v1 = POP();
        v2 = POP();
        PUSH_I(v1.i / v2.i);
        NEXT;
    IMOD:
        v1 = POP();
        v2 = POP();
        PUSH_I(v1.i % v2.i);
        NEXT;
    DNEG:
        v1 = POP();
        PUSH_D(-v1.d);
        NEXT;
    INEG:
        v1 = POP();
        PUSH_I(-v1.i);
        NEXT;
    IAOR:
        v1 = POP();
        v2 = POP();
        PUSH_I(v1.i | v2.i);
        NEXT;
    IAAND:
        v1 = POP();
        v2 = POP();
        PUSH_I(v1.i & v2.i);
        NEXT;
    IAXOR:
        v1 = POP();
        v2 = POP();
        PUSH_I(v1.i ^ v2.i);
        NEXT;
    IPRINT:
        v1 = POP();
        cout << v1.i;
        NEXT;
    DPRINT:
        v1 = POP();
        cout << v1.d;
        NEXT;
    SPRINT:
        v1 = POP();
        cout << (char*)v1.s;
        NEXT;
    I2D:
        v1 = POP();
        PUSH_D(v1.i);
        NEXT;
    D2I:
        v1 = POP();
        PUSH_I(v1.d);
        NEXT;
    S2I:
        v1 = POP();
        PUSH_I(v1.s);
        NEXT;
    SWAP:
        v1 = POP();
        v2 = POP();
        PUSH(v1);
        PUSH(v2);
        NEXT;
    POP:
        stack_top--;
        NEXT;
    LOADDVAR0:
    LOADIVAR0:
    LOADSVAR0:
        CHECK_STACK_OF(1)
        PUSH( VAR(0, 0) );
        NEXT;
    LOADDVAR1:
    LOADIVAR1:
    LOADSVAR1:
        CHECK_STACK_OF(1)
        PUSH( VAR(0, 1) );
        NEXT;
    LOADDVAR2:
    LOADIVAR2:
    LOADSVAR2:
        CHECK_STACK_OF(1)
        PUSH( VAR(0, 2) );
        NEXT;
    LOADDVAR3:
    LOADIVAR3:
    LOADSVAR3:
        CHECK_STACK_OF(1)
        PUSH( VAR(0, 3) );
        NEXT;
    STOREDVAR0:
    STOREIVAR0:
    STORESVAR0:
        v1 = POP();
        VAR(0, 0) = v1;
        NEXT;
    STOREDVAR1:
    STOREIVAR1:
    STORESVAR1:
        v1 = POP();
        VAR(0, 1) = v1;
        NEXT;
    STOREDVAR2:
    STOREIVAR2:
    STORESVAR2:
        v1 = POP();
        VAR(0, 2) = v1;
        NEXT;
    STOREDVAR3:
    STOREIVAR3:
    STORESVAR3:
        v1 = POP();
        VAR(0, 3) = v1;
        NEXT;
    LOADDVAR:
    LOADIVAR:
    LOADSVAR:
        CHECK_STACK_OF(1)
        variable_p = GET_DATA_16();
        PUSH( VAR(0, variable_p) );
        NEXT;
    STOREDVAR:
    STOREIVAR:
    STORESVAR:
        variable_p = GET_DATA_16();
        v1 = POP();
        VAR(0, variable_p) = v1;
        NEXT;
    LOADCTXDVAR:
    LOADCTXIVAR:
    LOADCTXSVAR:
        CHECK_STACK_OF(1)
        context_p = GET_DATA_16();
        variable_p = GET_DATA_16();
        PUSH( VAR(context_p, variable_p) );
        NEXT;
    STORECTXDVAR:
    STORECTXIVAR:
    STORECTXSVAR:
        context_p = GET_DATA_16();
        variable_p = GET_DATA_16();
        v1 = POP();
        VAR(context_p, variable_p) = v1;
        NEXT;
    DCMP:
        v1 = POP();
        v2 = POP();
        PUSH_I( (v1.d > v2.d) - (v1.d < v2.d) );
        NEXT;
    ICMP:
        v1 = POP();
        v2 = POP();
        PUSH_I(v1.i - v2.i);
        NEXT;
    JA:
        index += PEEK_DATA_S16();
        NEXT;
    IFICMPNE:
        v1 = POP();
        v2 = POP();
        if (v1.i != v2.i) {
            index += PEEK_DATA_S16();
        } else {
            index += 2;
        }
        NEXT;
    IFICMPE:
        v1 = POP();
        v2 = POP();
        if (v1.i == v2.i) {
            index += PEEK_DATA_S16();
        } else {
            index += 2;
        }
        NEXT;
    IFICMPG:
        v1 = POP();
        v2 = POP();
        if (v1.i > v2.i) {
            index += PEEK_DATA_S16();
        } else {
            index += 2;
        }
        NEXT;
    IFICMPGE:
        v1 = POP();
        v2 = POP();
        if (v1.i >= v2.i) {
            index += PEEK_DATA_S16();
        } else {
            index += 2;
        }
        NEXT;
    IFICMPL:
        v1 = POP();
        v2 = POP();
        if (v1.i < v2.i) {
            index += PEEK_DATA_S16();
        } else {
            index += 2;
        }
        NEXT;
    IFICMPLE:
        v1 = POP();
        v2 = POP();
        if (v1.i <= v2.i) {
            index += PEEK_DATA_S16();
        } else {
            index += 2;
        }
        NEXT;
    DUMP:
        v1 = TOP();
        cout << "DUMP: " << v1.i << " " << v1.d << " " << v1.s << endl;
        NEXT;
    STOP:
        RETURN();
    CALL:
        fun_id = GET_DATA_16();
        fun_data = getFunctionData( fun_id );
        call_stack.back().index = index;
        call_stack.push_back(CallData(fun_id, fun_context[fun_id], fun_data.stack_size));
        fun_context[fun_id] = context_stack.size();
        context_stack.resize(context_stack.size() + fun_data.stack_size);
        data = fun_data.fun->bytecode()->getData();
        index = 0;
        NEXT;
    CALLNATIVE:
        fun_id = GET_DATA_16();
        fun_data = getFunctionData( fun_id );
        native_fun = fun_data.native_fun;

        intIdx = 0;
        doubleIdx = 0;
        for (uint16_t i = 0; i < native_fun->parametersNumber(); ++i) {
            switch (native_fun->parameterType(i)) {
                case VT_INT:
                    intArgs[native_fun->intParamsNum() - intIdx - 1] = POP().i;
                    intIdx++;
                    break;
                case VT_STRING:
                    intArgs[native_fun->intParamsNum() - intIdx - 1] = (int64_t) POP().s;
                    intIdx++;
                    break;
                case VT_DOUBLE:
                    doubleArgs[native_fun->doubleParamsNum() - doubleIdx - 1] = POP().d;
                    doubleIdx++;
                    break;
                default:
                    RETURN_ERR("Illegal native parameter type: " + type2str(native_fun->returnType()));
            }
        }

// 6 int parameters + 8 double is enough for everyone
#define CALL_NATIVE_FUNC(PTR, RET_TYPE, RET_EXPR)                                      \
    __asm__("mov %0, %%rdi;"::"r"(intArgs[0]));                                 \
    __asm__("mov %0, %%rsi;"::"r"(intArgs[1]));                                 \
    __asm__("mov %0, %%rdx;"::"r"(intArgs[2]));                                 \
    __asm__("mov %0, %%rcx;"::"r"(intArgs[3]));                                 \
    __asm__("mov %0, %%r8;"::"r"(intArgs[4]));                                  \
    __asm__("mov %0, %%r9;"::"r"(intArgs[5]));                                  \
    RET_EXPR (*(RET_TYPE(*)                                                     \
        (double, double, double, double, double, double, double, double))PTR)   \
        (doubleArgs[0], doubleArgs[1], doubleArgs[2], doubleArgs[3],            \
            doubleArgs[4], doubleArgs[5], doubleArgs[6], doubleArgs[7])

        switch (native_fun->returnType()) {
            case VT_INT:
                CALL_NATIVE_FUNC(native_fun->ptr(), int64_t, v1.i=);
                PUSH(v1);
                break;
            case VT_STRING:
                CALL_NATIVE_FUNC(native_fun->ptr(), int64_t, v1.s=(char*));
                PUSH(v1);
                break;
            case VT_DOUBLE:
                CALL_NATIVE_FUNC(native_fun->ptr(), double, v1.d=);
                PUSH(v1);
                break;
            case VT_VOID:
                CALL_NATIVE_FUNC(native_fun->ptr(), void,);
                break;
            default:
                RETURN_ERR("Illegal native return type: " + type2str(native_fun->returnType()));
        }

#undef CALL_NATIVE_FUNC

        NEXT;
    RETURN:
        call_data = pop<CallData>(call_stack);
        context_stack.resize(context_stack.size() - call_data.stack_size);
        index = call_stack.back().index;
        fun_context[call_data.id] = call_data.last_stack;
        data = getFunctionData( call_stack.back().id ).fun->bytecode()->getData();
        NEXT;
    BREAK:
        NEXT;

    RETURN_ERR("End of instructions");
}

}
