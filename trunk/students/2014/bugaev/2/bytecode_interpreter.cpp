#include "bytecode_interpreter.hpp"
#include "interpreter_code.hpp"
#include "ast.h"

#include <iostream>


namespace mathvm
{

BytecodeInterpreter::BytecodeInterpreter(InterpreterCodeImpl *code):
    m_code(code),
    m_locals(code)
{
    m_locals.push(0);
    pushFunc(AstFunction::top_name);
    pushBci();
}


void BytecodeInterpreter::interpret()
{
    while (bci() < bc()->length()) {
        bool jmp = false;
        Value first;
        Value second;

        switch (bc()->getInsn(bci())) {
        case BC_STOP:
            return;

        case BC_DLOAD0:
            pushValue(0.0);
            break;
        case BC_ILOAD0:
            pushValue<int64_t>(0);
            break;
        case BC_DLOAD1:
            pushValue(1.0);
            break;
        case BC_ILOAD1:
            pushValue<int64_t>(1);
            break;
        case BC_DLOADM1:
            pushValue(-1.0);
            break;
        case BC_ILOADM1:
            pushValue<int64_t>(-1);
            break;
        case BC_DLOAD:
            pushValue(bc()->getDouble(bci() + 1));
            break;
        case BC_ILOAD:
            pushValue(bc()->getInt64(bci() + 1));
            break;
        case BC_SLOAD:
            pushValue(bc()->getUInt16(bci() + 1));
            break;

        case BC_CALL:
            m_locals.push(bc()->getUInt16(bci() + 1));
            pushFunc(bc()->getUInt16(bci() + 1));
            pushBci();
            jmp = true;
            break;
        case BC_RETURN:
            m_locals.pop();
            popFunc();
            popBci();
            //jmp = true;
            break;

#define LOAD_VAR_N(type, n) \
    pushValue(m_locals.load(1, n).type()); \
    break;
        case BC_LOADDVAR0:
            LOAD_VAR_N(doubleValue, 0)
        case BC_LOADDVAR1:
            LOAD_VAR_N(doubleValue, 1)
        case BC_LOADDVAR2:
            LOAD_VAR_N(doubleValue, 2)
        case BC_LOADDVAR3:
            LOAD_VAR_N(doubleValue, 3)
        case BC_LOADIVAR0:
            LOAD_VAR_N(intValue, 0)
        case BC_LOADIVAR1:
            LOAD_VAR_N(intValue, 1)
        case BC_LOADIVAR2:
            LOAD_VAR_N(intValue, 2)
        case BC_LOADIVAR3:
            LOAD_VAR_N(intValue, 3)
        case BC_LOADSVAR0:
            LOAD_VAR_N(constId, 0)
        case BC_LOADSVAR1:
            LOAD_VAR_N(constId, 1)
        case BC_LOADSVAR2:
            LOAD_VAR_N(constId, 2)
        case BC_LOADSVAR3:
            LOAD_VAR_N(constId, 3)
#undef LOAD_VAR_N

#define STORE_VAR_N(type, n) \
    pushValue(m_locals.load(1, n).type()); \
    break;
        case BC_STOREDVAR0:
            STORE_VAR_N(doubleValue, 0)
        case BC_STOREDVAR1:
            STORE_VAR_N(doubleValue, 1)
        case BC_STOREDVAR2:
            STORE_VAR_N(doubleValue, 2)
        case BC_STOREDVAR3:
            STORE_VAR_N(doubleValue, 3)
        case BC_STOREIVAR0:
            STORE_VAR_N(intValue, 0)
        case BC_STOREIVAR1:
            STORE_VAR_N(intValue, 1)
        case BC_STOREIVAR2:
            STORE_VAR_N(intValue, 2)
        case BC_STOREIVAR3:
            STORE_VAR_N(intValue, 3)
        case BC_STORESVAR0:
            STORE_VAR_N(constId, 0)
        case BC_STORESVAR1:
            STORE_VAR_N(constId, 1)
        case BC_STORESVAR2:
            STORE_VAR_N(constId, 2)
        case BC_STORESVAR3:
            STORE_VAR_N(constId, 3)
#undef STORE_VAR_N

#define LOAD_VAR(type) \
    pushValue(m_locals.load(1, bc()->getUInt16(bci() + 1)).type()); \
    break;
        case BC_LOADDVAR:
            LOAD_VAR(doubleValue)
        case BC_LOADIVAR:
            LOAD_VAR(intValue)
        case BC_LOADSVAR:
            LOAD_VAR(constId)
#undef LOAD_CTX_VAR

#define STORE_VAR(type) \
    m_locals.store(popValue().type(), 1, bc()->getUInt16(bci() + 1)); \
    break;
        case BC_STOREDVAR:
            STORE_VAR(doubleValue)
        case BC_STOREIVAR:
            STORE_VAR(intValue)
        case BC_STORESVAR:
            STORE_VAR(constId)
#undef STORE_VAR

#define LOAD_CTX_VAR(type) \
    pushValue(m_locals.load(bc()->getUInt16(bci() + 1), \
                            bc()->getUInt16(bci() + 3)).type()); \
    break;
        case BC_LOADCTXDVAR:
            LOAD_CTX_VAR(doubleValue)
        case BC_LOADCTXIVAR:
            LOAD_CTX_VAR(intValue)
        case BC_LOADCTXSVAR:
            LOAD_CTX_VAR(constId)
#undef LOAD_CTX_VAR

#define STORE_CTX_VAR(type) \
    m_locals.store(popValue().type(), \
                   bc()->getUInt16(bci() + 1), \
                   bc()->getUInt16(bci() + 3)); \
    break;
        case BC_STORECTXDVAR:
            STORE_CTX_VAR(doubleValue)
        case BC_STORECTXIVAR:
            STORE_CTX_VAR(intValue)
        case BC_STORECTXSVAR:
            STORE_CTX_VAR(constId)
#undef STORE_CTX_VAR

#define CMP_JMP(op) \
    first = popValue(); \
    second = popValue(); \
    if (first.intValue() op second.intValue()) { \
        bci() += bc()->getInt16(bci() + 1) + 1; \
        jmp = true; \
    } \
    break;
        case BC_IFICMPNE:
            CMP_JMP(!=)
        case BC_IFICMPE:
            CMP_JMP(==)
        case BC_IFICMPG:
            CMP_JMP(>)
        case BC_IFICMPGE:
            CMP_JMP(>=)
        case BC_IFICMPL:
            CMP_JMP(<)
        case BC_IFICMPLE:
            CMP_JMP(<=)
#undef CMP_JMP

        case BC_JA:
            bci() += bc()->getInt16(bci() + 1) + 1;
            jmp = true;
            break;

#define BINARY_OP(type, op) \
    first = popValue(); \
    second = popValue(); \
    pushValue(first.type() op second.type()); \
    break;
        case BC_DADD:
            BINARY_OP(doubleValue, +)
        case BC_IADD:
            BINARY_OP(intValue, +)
        case BC_DSUB:
            BINARY_OP(doubleValue, -)
        case BC_ISUB:
            BINARY_OP(intValue, -)
        case BC_DMUL:
            BINARY_OP(doubleValue, *)
        case BC_IMUL:
            BINARY_OP(intValue, *)
        case BC_DDIV:
            BINARY_OP(doubleValue, /)
        case BC_IDIV:
            BINARY_OP(intValue, /)
        case BC_IMOD:
            BINARY_OP(intValue, %)
        case BC_IAOR:
            BINARY_OP(intValue, |)
        case BC_IAAND:
            BINARY_OP(intValue, &)
        case BC_IAXOR:
            BINARY_OP(intValue, ^)
#undef BINARY_OP

#define CMP(type) \
    first = popValue(); \
    second = popValue(); \
    if (first.type() < second.type()) \
        pushValue<int64_t>(-1); \
    else if (first.type() == second.type()) \
        pushValue<int64_t>(0); \
    else \
        pushValue<int64_t>(1); \
    break;
        case BC_DCMP:
            CMP(doubleValue)
        case BC_ICMP:
            CMP(intValue)
#undef CMP

        case BC_DNEG:
            pushValue(-popValue().doubleValue());
            break;
        case BC_INEG:
            pushValue(-popValue().intValue());
            break;

        case BC_I2D:
            pushValue<double>(popValue().intValue());
            break;
        case BC_D2I:
            pushValue<int64_t>(popValue().doubleValue());
            break;
        case BC_SWAP:
            first = popValue();
            second = popValue();
            pushValue(first);
            pushValue(second);
            break;
        case BC_POP:
            popValue();
            break;

        case BC_IPRINT:
            writeValue(std::cout, popValue(), VT_INT);
            break;
        case BC_DPRINT:
            writeValue(std::cout, popValue(), VT_DOUBLE);
            break;
        case BC_SPRINT:
            writeValue(std::cout, popValue(), VT_STRING);
            break;
        case BC_DUMP:
            first = popValue();
            writeValue(std::cerr, first, first.type());
            break;

        case BC_CALLNATIVE:
        case BC_BREAK:
        case BC_S2I:
        case BC_SLOAD0:
        default:
            throw BytecodeException("Unsupported bytecode");
        }

        moveBci(jmp);
    }

    throw BytecodeException("STOP bytecode is not found");
}


void BytecodeInterpreter::moveBci(bool ignoring)
{
    static size_t sizes[] = {
#define BYTECODE_SIZE(b, d, size) size,
        FOR_BYTECODES(BYTECODE_SIZE)
#undef BYTECODE_SIZE
    };

    if (ignoring)
        return;

    bci() += sizes[bc()->getInsn(bci())];
}


void BytecodeInterpreter::writeValue(
        std::ostream &out,
        BytecodeInterpreter::Value const &value,
        VarType type)
{
    switch (type) {
    case VT_INT:
        out << value.intValue();
        break;
    case VT_DOUBLE:
        out << value.doubleValue();
        break;
    case VT_STRING:
        out << m_code->constantById(value.constId());
        break;
    default:
        throw BytecodeException("Type is not printable");
    }
}

}
