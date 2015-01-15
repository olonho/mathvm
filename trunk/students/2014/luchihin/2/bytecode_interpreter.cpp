#include "bytecode_interpreter.h"

#include <string>
#include <iostream>

using std::string;
using std::cout;

using namespace mathvm;

Status *InterpreterCodeImpl::execute(vector<Var *> &vars) {
    m_current_scope = 0;
    call_function(0);
    while(true) {
        switch(bytecode()->getInsn(ip()++)) {
            case BC_INVALID:
                return Status::Error("Invalid operation found");
            case BC_DLOAD:
                m_stack.push(get_double());
                break;
            case BC_ILOAD:
                m_stack.push(get_int64());
                break;
            case BC_SLOAD:
                m_stack.push(constantById(get_uint16()).data());
                break;
            case BC_DLOAD0:
                m_stack.push(0.0);
                break;
            case BC_ILOAD0:
                m_stack.push((int64_t)0);
                break;
            case BC_SLOAD0:
                m_stack.push((int64_t)0);
                break;
            case BC_DLOAD1:
                m_stack.push(1.0);
                break;
            case BC_ILOAD1:
                m_stack.push((int64_t)1);
                break;
            case BC_DLOADM1:
                m_stack.push(-1.0);
                break;
            case BC_ILOADM1:
                m_stack.push((int64_t)(-1));
                break;

            case BC_DADD:
                double_bin_op(tADD);
                break;
            case BC_IADD:
                int_bin_op(tADD);
                break;
            case BC_DSUB:
                double_bin_op(tSUB);
                break;
            case BC_ISUB:
                int_bin_op(tSUB);
                break;
            case BC_DMUL:
                double_bin_op(tMUL);
                break;
            case BC_IMUL:
                int_bin_op(tMUL);
                break;
            case BC_DDIV:
                double_bin_op(tDIV);
                break;
            case BC_IDIV:
                int_bin_op(tDIV);
                break;
            case BC_IMOD:
                int_bin_op(tMOD);
                break;
            case BC_IAAND:
                int_bin_op(tAAND);
                break;
            case BC_IAOR:
                int_bin_op(tAOR);
                break;
            case BC_IAXOR:
                int_bin_op(tAXOR);
                break;
            case BC_DNEG:
                m_stack.push(-pop_stack().get_double());
                break;
            case BC_INEG:
                m_stack.push(-pop_stack().get_int());
                break;

            case BC_I2D:
                m_stack.push((double)pop_stack().get_int());
                break;
            case BC_D2I:
                m_stack.push((int64_t)pop_stack().get_double());
                break;
            case BC_S2I:
                m_stack.push((int64_t)pop_stack().get_string_ptr());
                break;

            case BC_SWAP: {
                var_holder a = pop_stack();
                var_holder b = pop_stack();
                m_stack.push(a);
                m_stack.push(b);
                break;
            }
            case BC_POP:
                m_stack.pop();
                break;

            case BC_IPRINT:
                cout << pop_stack().get_int();
                break;
            case BC_DPRINT:
                cout << pop_stack().get_double();
                break;
            case BC_SPRINT:
                cout << pop_stack().get_string();
                break;

            case BC_LOADIVAR0:
            case BC_LOADDVAR0:
            case BC_LOADSVAR0:
                load_var(0);
                break;
            case BC_LOADIVAR1:
            case BC_LOADDVAR1:
            case BC_LOADSVAR1:
                load_var(1);
                break;
            case BC_LOADIVAR2:
            case BC_LOADDVAR2:
            case BC_LOADSVAR2:
                load_var(2);
                break;
            case BC_LOADIVAR3:
            case BC_LOADDVAR3:
            case BC_LOADSVAR3:
                load_var(3);
                break;
            case BC_LOADIVAR:
            case BC_LOADDVAR:
            case BC_LOADSVAR:
                load_var(get_uint16());
                break;
            case BC_LOADCTXIVAR:
            case BC_LOADCTXDVAR:
            case BC_LOADCTXSVAR: {
                uint16_t a = get_uint16();
                uint16_t b = get_uint16();
                load_var(a, b);
            }
                break;

            case BC_STOREIVAR0:
            case BC_STOREDVAR0:
            case BC_STORESVAR0:
                store_var(0);
                break;
            case BC_STOREIVAR1:
            case BC_STOREDVAR1:
            case BC_STORESVAR1:
                store_var(1);
                break;
            case BC_STOREIVAR2:
            case BC_STOREDVAR2:
            case BC_STORESVAR2:
                store_var(2);
                break;
            case BC_STOREIVAR3:
            case BC_STOREDVAR3:
            case BC_STORESVAR3:
                store_var(3);
                break;
            case BC_STOREIVAR:
            case BC_STOREDVAR:
            case BC_STORESVAR:
                store_var(get_uint16());
                break;
            case BC_STORECTXIVAR:
            case BC_STORECTXDVAR:
            case BC_STORECTXSVAR: {
                uint16_t a = get_uint16();
                uint16_t b = get_uint16();
                store_var(a, b);
            }
                break;

            case BC_DCMP:
                double_bin_op(tEQ);
                break;
            case BC_ICMP:
                int_bin_op(tEQ);
                break;

            case BC_JA:
                ip() += get_int16();
                break;
            case BC_IFICMPE: {
                int32_t a = pop_stack().get_int();
                int32_t b = pop_stack().get_int();
                ip() += (a == b ? get_int16() : 2);
            }
                break;
            case BC_IFICMPG: {
                int32_t a = pop_stack().get_int();
                int32_t b = pop_stack().get_int();
                ip() += (a > b ? get_int16() : 2);
            }
                break;
            case BC_IFICMPL: {
                int32_t a = pop_stack().get_int();
                int32_t b = pop_stack().get_int();
                ip() += (a < b ? get_int16() : 2);
            }
                break;
            case BC_IFICMPNE: {
                int32_t a = pop_stack().get_int();
                int32_t b = pop_stack().get_int();
                ip() += (a != b ? get_int16() : 2);
            }
                break;
            case BC_IFICMPGE: {
                int32_t a = pop_stack().get_int();
                int32_t b = pop_stack().get_int();
                ip() += (a >= b ? get_int16() : 2);
            }
                break;
            case BC_IFICMPLE: {
                int32_t a = pop_stack().get_int();
                int32_t b = pop_stack().get_int();
                ip() += (a <= b ? get_int16() : 2);
            }
                break;

            case BC_STOP:
                return 0;
            case BC_CALL:
                call_function(get_uint16());
                break;
            case BC_CALLNATIVE:
                return Status::Error("Unsupported operation");

            case BC_RETURN: {
                Scope* parentScope = m_current_scope->parent;
                delete m_current_scope;
                m_current_scope = parentScope;
                break;
            }

            default:
                return Status::Error("Unsupported operation");
        }
    }
    return 0;
}

void InterpreterCodeImpl::double_bin_op(TokenKind op) {
    double a = pop_stack().get_double();
    double b = pop_stack().get_double();
    switch(op) {
        case tADD:
            m_stack.push(a + b);
            break;
        case tSUB:
            m_stack.push(a - b);
            break;
        case tMUL:
            m_stack.push(a * b);
            break;
        case tDIV:
            m_stack.push(a / b);
            break;
        case tEQ:
            m_stack.push((int64_t)(a == b));
            break;
        default:
            throw string("Unsupported double operation ") + tokenOp(op);
    }
}

void InterpreterCodeImpl::int_bin_op(TokenKind op) {
    int64_t a = pop_stack().get_int();
    int64_t b = pop_stack().get_int();
    switch(op) {
        case tADD:
            m_stack.push(a + b);
            break;
        case tSUB:
            m_stack.push(a - b);
            break;
        case tMUL:
            m_stack.push(a * b);
            break;
        case tDIV:
            m_stack.push(a / b);
            break;
        case tMOD:
            m_stack.push(a % b);
            break;
        case tAAND:
            m_stack.push(a & b);
            break;
        case tAXOR:
            m_stack.push(a ^ b);
            break;
        case tAOR:
            m_stack.push(a | b);
            break;
        case tEQ:
            m_stack.push((int64_t)(a == b));
            break;
        default:
            throw string("Unsupported int operation ") + tokenOp(op);
    }
}

void InterpreterCodeImpl::load_var(uint16_t index) {
    m_stack.push(m_current_scope->vars[index]);
}

void InterpreterCodeImpl::load_var(uint16_t cid, uint16_t index) {
    Scope* scope = m_current_scope->parent;
    while(scope && scope->function->id() != cid)
        scope = scope->parent;
    if(!scope) throw string("Context not found");
    m_stack.push(scope->vars[index]);
}

void InterpreterCodeImpl::store_var(uint32_t index) {
    m_current_scope->vars[index] = pop_stack();
}

void InterpreterCodeImpl::store_var(uint16_t cid, uint16_t index) {
    Scope* scope = m_current_scope->parent;
    while(scope && scope->function->id() != cid)scope = scope->parent;
    if(!scope) throw string("Context not found");
    scope->vars[index] = pop_stack();
}
