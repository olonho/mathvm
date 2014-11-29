#include "bytecode_interpreter.h"

#include <functional>

template<typename T>
struct Cmp {
    Int operator()(T a, T b) const {
        return (Int) ((a > b) - (a < b));
    }
};

#ifndef STACK_SIZE
#define STACK_SIZE 1024 * 1024 * 2
#endif
#ifndef CONTEXT_SIZE
#define CONTEXT_SIZE 1024 * 1024 * 8
#endif

std::vector<StackValue> context(CONTEXT_SIZE, 0xCCCCCCCCL);   /* 64mb */
std::vector<StackValue>   stack(  STACK_SIZE, 0xCCCCCCCCL);   /* 16mb */


namespace mathvm {

/*
 * Computed goto inspired by
 * http://eli.thegreenplace.net/2012/07/12/computed-goto-for-efficient-dispatch-tables
*/
Status* InterpreterCodeImpl::execute(vector<Var*>& ignored) {

#define MACRO_DRIVEN_DEVELOPMENT
#define DISPATCH() goto *dispatchTable[code->getInsn(bci++)]
#define ERROR(msg) return Status::Error(msg)
#define PUSH(v)    stack[++tos] = v
#define TOP()      stack[tos]
#define PREV_TOP() stack[tos - 1]
#define POP()      stack[tos--]
#define GET_AND_INCREMENT(x, inc) ((x += inc) - inc)
#define NEXT_DOUBLE() code->getDouble(GET_AND_INCREMENT(bci, 8))
#define NEXT_INT64()  code->getInt64 (GET_AND_INCREMENT(bci, 8))
#define NEXT_UINT16() code->getUInt16(GET_AND_INCREMENT(bci, 2))
#define VAR(funId, localIndex) context[funContextStart[funId] + localIndex]
#define JUMP() bci += code->getInt16(bci)

#define UNARY_OP(Type, FUN)  \
    top = POP();             \
    PUSH(FUN<Type>()(top))

#define BINARY_OP(Type, FUN) \
    top     = POP();         \
    prevTop = POP();         \
    PUSH(FUN<Type>()(top, prevTop))

#define CMP_OP(FUN)          \
    top     = POP();         \
    prevTop = POP();         \
    FUN<Int>()(top, prevTop) ? JUMP() : bci += 2


    if (!ignored.empty())
        ERROR("Execution with vars is not implemented!");

    static void* dispatchTable[] = {
#define COMPUTED_GOTO(b, d, l) &&b,

        FOR_BYTECODES(COMPUTED_GOTO)
#undef COMPUTED_GOTO
    };

    uint32_t bci = 0;
    BytecodeFunction* fun = getFunction(0);
    Bytecode* code = fun->bytecode();

    vector<uint32_t> funContextStart(functionsNumber());
    uint32_t tos = 0;
    uint32_t contextTop = fun->localsNumber();
    uint16_t funId = 0;
    uint16_t ctx = 0;
    uint16_t localIndex = 0;
    StackFrame frame;
    std::vector<StackFrame> callStack;
    callStack.reserve(1 << 16);
    callStack.push_back({0, 0, 0});

    /*register*/ StackValue     top /*asm("r12")*/;
    /*register*/ StackValue prevTop /*asm("r13")*/;


    DISPATCH();
 INVALID:
    ERROR("Invalid bytecode");
 DLOAD:
    PUSH(NEXT_DOUBLE());
    DISPATCH();
 ILOAD:
    PUSH(NEXT_INT64());
    DISPATCH();
 SLOAD:
    PUSH(constantById(NEXT_UINT16()).c_str());
    DISPATCH();
 DLOAD0:
    PUSH(0.0);
    DISPATCH();
 ILOAD0:
    PUSH(0L);
    DISPATCH();
 SLOAD0:
    PUSH(constantById(0).c_str());
    DISPATCH();
 DLOAD1:
    PUSH(1.0);
    DISPATCH();
 ILOAD1:
    PUSH(1L);
    DISPATCH();
 DLOADM1:
    PUSH(-1.0);
    DISPATCH();
 ILOADM1:
    PUSH(-1L);
    DISPATCH();
 DADD:
    BINARY_OP(Double, std::plus);
    DISPATCH();
 IADD:
    BINARY_OP(Int,    std::plus);
    DISPATCH();
 DSUB:
    BINARY_OP(Double, std::minus);
    DISPATCH();
 ISUB:
    BINARY_OP(Int,    std::minus);
    DISPATCH();
 DMUL:
    BINARY_OP(Double, std::multiplies);
    DISPATCH();
 IMUL:
    BINARY_OP(Int,    std::multiplies);
    DISPATCH();
 DDIV:
    BINARY_OP(Double, std::divides);
    DISPATCH();
 IDIV:
    BINARY_OP(Int,    std::divides);
    DISPATCH();
 IMOD:
    BINARY_OP(Int,    std::modulus);
    DISPATCH();
 DNEG:
    UNARY_OP(Double,  std::negate);
    DISPATCH();
 INEG:
    UNARY_OP(Int,     std::negate);
    DISPATCH();
 IAOR:
    BINARY_OP(Int,    std::bit_or);
    DISPATCH();
 IAAND:
    BINARY_OP(Int,    std::bit_and);
    DISPATCH();
 IAXOR:
    BINARY_OP(Int,    std::bit_xor);
    DISPATCH();
 IPRINT:
    cout << POP().i;
    DISPATCH();
 DPRINT:
    cout << POP().d;
    DISPATCH();
 SPRINT:
    cout << POP().s;
    DISPATCH();
 I2D:
    top = POP();
    PUSH((Double) top.i);
    DISPATCH();
 D2I:
    top = POP();
    PUSH((Int) top.d);
    DISPATCH();
 S2I:
    ERROR("Bytecode S2I is deprecated");
 SWAP:
    std::swap(TOP(), PREV_TOP());
    DISPATCH();
 POP:
    --tos;
    DISPATCH();
 LOADDVAR0:
 LOADIVAR0:
 LOADSVAR0:
    PUSH(VAR(funId, 0));
    DISPATCH();
 LOADDVAR1:
 LOADIVAR1:
 LOADSVAR1:
    PUSH(VAR(funId, 1));
    DISPATCH();
 LOADDVAR2:
 LOADIVAR2:
 LOADSVAR2:
    PUSH(VAR(funId, 2));
    DISPATCH();
 LOADDVAR3:
 LOADIVAR3:
 LOADSVAR3:
    PUSH(VAR(funId, 3));
    DISPATCH();
 STOREDVAR0:
 STOREIVAR0:
 STORESVAR0:
    VAR(funId, 0) = POP();
    DISPATCH();
 STOREDVAR1:
 STOREIVAR1:
 STORESVAR1:
    VAR(funId, 1) = POP();
    DISPATCH();
 STOREDVAR2:
 STOREIVAR2:
 STORESVAR2:
    VAR(funId, 2) = POP();
    DISPATCH();
 STOREDVAR3:
 STOREIVAR3:
 STORESVAR3:
    VAR(funId, 3) = POP();
    DISPATCH();
 LOADDVAR:
 LOADIVAR:
 LOADSVAR:
    localIndex = NEXT_UINT16();
    PUSH(VAR(funId, localIndex));
    DISPATCH();
 STOREDVAR:
 STOREIVAR:
 STORESVAR:
    localIndex = NEXT_UINT16();
    VAR(funId, localIndex) = POP();
    DISPATCH();
 LOADCTXDVAR:
 LOADCTXIVAR:
 LOADCTXSVAR:
    ctx = NEXT_UINT16();
    localIndex = NEXT_UINT16();
    PUSH(VAR(ctx, localIndex));
    DISPATCH();
 STORECTXDVAR:
 STORECTXIVAR:
 STORECTXSVAR:
    ctx = NEXT_UINT16();
    localIndex = NEXT_UINT16();
    VAR(ctx, localIndex) = POP();
    DISPATCH();
 DCMP:
    BINARY_OP(Double, Cmp);
    DISPATCH();
 ICMP:
    BINARY_OP(Int, Cmp);
    DISPATCH();
 JA:
    JUMP();
    DISPATCH();
 IFICMPNE:
    CMP_OP(std::not_equal_to);
    DISPATCH();
 IFICMPE:
    CMP_OP(std::equal_to);
    DISPATCH();
 IFICMPG:
    CMP_OP(std::greater);
    DISPATCH();
 IFICMPGE:
    CMP_OP(std::greater_equal);
    DISPATCH();
 IFICMPL:
    CMP_OP(std::less);
    DISPATCH();
 IFICMPLE:
    CMP_OP(std::less_equal);
    DISPATCH();
 DUMP:
    fprintf(stderr, "Dump top: %li\n", TOP().i);
    DISPATCH();
 STOP:
    assert(callStack.size() == 1);
    return Status::Ok();
 CALLNATIVE:
    const Signature * signature;
    const string * name;
    const void* address = nativeById(NEXT_UINT16(), &signature, &name);
    const size_t argsNumber = signature->size() - 1;
    tos -= argsNumber;
    StackValue* argsStart = &stack[tos + 1];

    switch (signature->operator[](0).first) {
    /*
     * Bug note: double should be treated separately
     */
    case VT_DOUBLE:
        PUSH(asmjit_cast<double (*)(StackValue *)>(address)(argsStart));
        break;
    case VT_INT:
    case VT_STRING:
        PUSH(asmjit_cast<StackValue (*)(StackValue *)>(address)(argsStart));
        break;
    case VT_VOID:
        asmjit_cast<void (*)(StackValue *)>(address)(argsStart);
        break;
    default:
        assert("Invalid native return type" && false);
    }

    DISPATCH();
 CALL:
    funId = NEXT_UINT16();
    fun = getFunction(funId);
    code = fun->bytecode();
    {
        if (tos + stackSize(funId) > STACK_SIZE)
            ERROR("stack overflow");
        if (contextTop + fun->localsNumber() > CONTEXT_SIZE)
            ERROR("*context* stack  overflow");
    }
    callStack.push_back({funId, funContextStart[funId], bci});
    bci = 0;
    funContextStart[funId] = contextTop;
    contextTop += fun->localsNumber();
    DISPATCH();
 RETURN:
    frame = callStack.back();
    callStack.pop_back();
    funId = callStack.back().functionId;
    fun = getFunction(funId);
    code = fun->bytecode();
    bci = frame.bci;
    funContextStart[frame.functionId] = frame.prevContextStart;
    contextTop -= getFunction(frame.functionId)->localsNumber();
    DISPATCH();
 BREAK:
    ERROR("Breakpoint is not implemented!");

    ERROR("No way!");

#undef DISPATCH
#undef ERROR
#undef PUSH
#undef TOP
#undef PREV_TOP
#undef POP
#undef GET_AND_INCREMENT
#undef NEXT_DOUBLE
#undef NEXT_INT64
#undef NEXT_UINT16
#undef VAR
#undef JUMP
#undef UNARY_OP
#undef BINARY_OP
#undef CMP_OP
}

}
