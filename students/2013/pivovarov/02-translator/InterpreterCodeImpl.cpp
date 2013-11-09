#include "InterpreterCodeImpl.h"

#include <avcall.h>

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
    NativeFunction_ * native_fun;
    vector<Data> params;
    av_alist alist;
    params.reserve(16);

    stack_top = 0;
    fun_data = getFunctionData(0);
    call_stack.push_back(CallData(0, fun_context[0], fun_data.stack_size));
    context_stack.resize(fun_data.stack_size);
    data = fun_data.fun->bytecode()->getData();
    index = 0;
    NEXT;

    INVALID:
        return new Status("BC_INVALID");
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
        PUSH(v2);
        PUSH(v1);
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
        return new Status();
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

        switch (native_fun->returnType()) {
            case VT_INT:
                av_start_int (alist, native_fun->ptr(), &v1.i);
                break;
            case VT_DOUBLE:
                av_start_double (alist, native_fun->ptr(), &v1.d);
                break;
            case VT_STRING:
                av_start_ptr (alist, native_fun->ptr(), char *, &v1.s);
                break;
            case VT_VOID:
                av_start_void (alist, native_fun->ptr());
                break;
            default:
                return new Status("Illegal native return type");
        }

        // Reverse params
        params.reserve(native_fun->parametersNumber());
        for (uint16_t i = 0; i < native_fun->parametersNumber(); ++i) {
            params[native_fun->parametersNumber() - i - 1] = POP();
        }

        for (uint16_t i = 0; i < native_fun->parametersNumber(); ++i) {
            switch (native_fun->parameterType(i)) {
                case VT_INT:
                    av_int (alist, params[i].i);
                    break;
                case VT_DOUBLE:
                    av_double (alist, params[i].d);
                    break;
                case VT_STRING:
                    av_ptr (alist, char *, params[i].s);
                    break;
                default:
                    return new Status("Illegal native parameter type");
            }
        }

        av_call (alist);

        switch (native_fun->returnType()) {
            case VT_INT:
                PUSH_I(v1.i);
                break;
            case VT_DOUBLE:
                PUSH_D(v1.d);
                break;
            case VT_STRING:
                PUSH_S(v1.s);
                break;
            case VT_VOID:
                break;
            default:
                return new Status("Illegal native return type");
        }

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

    return new Status("End of instructions");
}

}
