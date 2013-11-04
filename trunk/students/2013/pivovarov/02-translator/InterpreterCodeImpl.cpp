#include "InterpreterCodeImpl.h"

namespace mathvm {

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
    Data(uint16_t s) : s(s) {}

    int64_t i;
    double d;
    uint16_t s;
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

#define PEEK_DATA_S16()                     \
    *(int16_t*)(data + index)

#define GET_DATA_D()                        \
    GET_DATA_64()
#define GET_DATA_I()                        \
    GET_DATA_64()
#define GET_DATA_S()                        \
    GET_DATA_16()

#define GET_INSN()                          \
    *(uint8_t*)(data + INC_8(index))

#define PUSH(V)                             \
    stack.push_back( V )
#define PUSH_D(V)                           \
    PUSH((double)V)
#define PUSH_I(V)                           \
    PUSH((int64_t)V)
#define PUSH_S(V)                           \
    PUSH((uint16_t)V)

#define POP()                               \
    pop<Data>( stack )
#define TOP()                               \
    stack.back()

#define VAR(CONTEXT, VARIABLE)              \
    context_stack[fun_context[CONTEXT] + VARIABLE]

#define NEXT                                \
    goto *(labels[GET_INSN()])

Status * InterpreterCodeImpl::execute() {
    uint32_t index;
    uint8_t * data;

    vector<CallData> call_stack;
    vector<uint32_t> fun_context;
    vector<Data> context_stack;
    vector<Data> stack;

    fun_context.resize(funsData.size());
    call_stack.reserve(1024);
    context_stack.reserve(4096);
    stack.reserve(1024);

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

    fun_data = getFunctionData(0);
    call_stack.push_back(CallData(0, fun_context[0], fun_data.stack_size));
    context_stack.resize(fun_data.stack_size);
    data = fun_data.fun->bytecode()->getData();
    index = 0;
    NEXT;

    INVALID:
        return new Status("BC_INVALID");
    DLOAD:
        PUSH( GET_DATA_D() );
        NEXT;
    ILOAD:
        PUSH( GET_DATA_I() );
        NEXT;
    SLOAD:
        PUSH( GET_DATA_S() );
        NEXT;
    DLOAD0:
        PUSH_D(0);
        NEXT;
    ILOAD0:
        PUSH_I(0);
        NEXT;
    SLOAD0:
        PUSH_S(0);
        NEXT;
    DLOAD1:
        PUSH_D(1);
        NEXT;
    ILOAD1:
        PUSH_I(1);
        NEXT;
    DLOADM1:
        PUSH_D(-1);
        NEXT;
    ILOADM1:
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
        cout << constantById(v1.s);
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
        POP();
        NEXT;
    LOADDVAR0:
    LOADIVAR0:
    LOADSVAR0:
        PUSH( VAR(0, 0) );
        NEXT;
    LOADDVAR1:
    LOADIVAR1:
    LOADSVAR1:
        PUSH( VAR(0, 1) );
        NEXT;
    LOADDVAR2:
    LOADIVAR2:
    LOADSVAR2:
        PUSH( VAR(0, 2) );
        NEXT;
    LOADDVAR3:
    LOADIVAR3:
    LOADSVAR3:
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
        return new Status("Native not supported");
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
