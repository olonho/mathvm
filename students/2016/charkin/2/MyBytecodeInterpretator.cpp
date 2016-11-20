#include <iostream>

#include "MyBytecodeInterpretator.h"

mathvm::Status* mathvm::MyBytecodeInterpretator::execute(std::vector<Var *> &vars) {
    BytecodeFunction* root = (BytecodeFunction*)functionById(0);
    _scope = new IScope(root);
    while (_scope->has_next_insn()) {
        Instruction insn = _scope->next_insn();
        if (!execute_insn(insn)) {
            break;
        }
    }
    return Status::Ok();
}

mathvm::StackItem::StackItem() {
    _type = VT_INVALID;
}

mathvm::StackItem::StackItem(int64_t i) {
    _type = VT_INT;
    _item._int = i;
}

mathvm::StackItem::StackItem(double d) {
    _type = VT_DOUBLE;
    _item._double = d;
}

mathvm::StackItem::StackItem(uint16_t s) {
    _type = VT_STRING;
    _item._str_id= s;
}

int64_t mathvm::StackItem::get_int() {
    if (!(_type == VT_INT))
        throw std::runtime_error("wrong stack item type");

    return _item._int;
}

double mathvm::StackItem::get_double() {
    if (!(_type == VT_DOUBLE))
        throw std::runtime_error("wrong stack item type");

    return _item._double;
}

uint16_t mathvm::StackItem::get_str_id() {
    if (!(_type == VT_STRING))
        throw std::runtime_error("wrong stack item type");

    return _item._str_id;
}

mathvm::IScope::IScope(BytecodeFunction *bytecode_func, IScope *parent) {
    _func = bytecode_func;
    _parent = parent;
    IP = 0;
    _vars.resize(bytecode_func->localsNumber() + 1);
}

mathvm::Instruction mathvm::IScope::next_insn() {
    return get_bytecode()->getInsn(IP++);
}

mathvm::IScope* mathvm::IScope::get_parent() {
    return _parent;
}

mathvm::Bytecode* mathvm::IScope::get_bytecode() {
    return _func->bytecode();
}

bool mathvm::IScope::has_next_insn() {
    return IP < get_bytecode()->length();
}

int64_t mathvm::IScope::get_int() {
    int64_t res = get_bytecode()->getInt64(IP);
    IP += sizeof(int64_t);
    return res;
}

uint16_t mathvm::IScope::get_uint16() {
    uint16_t res = get_bytecode()->getUInt16(IP);
    IP += sizeof(uint16_t);
    return res;
}

double mathvm::IScope::get_double() {
    double res = get_bytecode()->getDouble(IP);
    IP += sizeof(double);
    return res;
}

mathvm::StackItem mathvm::IScope::get_var(uint16_t id) {
    if (_vars.size() <= id)
        throw std::runtime_error("wrong var id");
    return _vars[id];
}

mathvm::StackItem mathvm::IScope::get_var(uint16_t scope_id, uint16_t var_id) {
    if (_func->scopeId() == scope_id) {
        return get_var(var_id);
    } else if (_parent != nullptr) {
        return _parent->get_var(scope_id, var_id);
    }
    throw std::runtime_error("wrong scope id");
    return StackItem();
}

void mathvm::IScope::store_var(StackItem var, uint16_t id) {
    if (_vars.size() <= id)
        throw std::runtime_error("wrong var id");
    _vars[id] = var;
}

void mathvm::IScope::store_var(StackItem var, uint16_t scope_id, uint16_t id) {
    if (_func->scopeId() == scope_id) {
        store_var(var, id);
    } else {
        if (_parent != nullptr) {
            _parent->store_var(var, scope_id, id);
        } else {
            throw std::runtime_error("wrong scope id");
        }
    }
}

void mathvm::IScope::jump() {
    uint32_t tmp = IP;
    IP += get_bytecode()->getInt16(tmp);
}

void mathvm::IScope::jump_if(bool condition) {
    if (condition) {
        jump();
    } else {
        IP += sizeof(int16_t);
    }
}

mathvm::StackItem mathvm::MyBytecodeInterpretator::get_top() {
    StackItem top = _stack.top();
    _stack.pop();
    return top;
}

void mathvm::MyBytecodeInterpretator::swap_tops() {
    StackItem top = _stack.top();
    _stack.pop();
    std::swap(top, _stack.top());
    _stack.push(top);
}


bool mathvm::MyBytecodeInterpretator::execute_insn(Instruction insn) {
    switch (insn) {
    case BC_DLOAD:
        _stack.push(_scope->get_double());
        break;
    case BC_ILOAD:
        _stack.push(_scope->get_int());
        break;
    case BC_SLOAD:
        _stack.push(_scope->get_uint16());
        break;

    case BC_DLOAD0:
        _stack.push((double)0.0);
        break;
    case BC_ILOAD0:
        _stack.push((int64_t)0);
        break;
    case BC_SLOAD0:
        _stack.push(makeStringConstant(""));
        break;

    case BC_DLOAD1:
        _stack.push((double)1.0);
        break;
    case BC_ILOAD1:
        _stack.push((int64_t)1);
        break;

    case BC_DLOADM1:
        _stack.push((double)-1.0);
        break;
    case BC_ILOADM1:
        _stack.push((int64_t)-1);
        break;

    case BC_DADD:
        _stack.push(get_top().get_double() + get_top().get_double());
        break;
    case BC_IADD:
        _stack.push(get_top().get_int() + get_top().get_int());
        break;

    case BC_DSUB:
        _stack.push(get_top().get_double() - get_top().get_double());
        break;
    case BC_ISUB:
        _stack.push(get_top().get_int() - get_top().get_int());
        break;

    case BC_DMUL:
        _stack.push(get_top().get_double() * get_top().get_double());
        break;
    case BC_IMUL:
        _stack.push(get_top().get_int() * get_top().get_int());
        break;

    case BC_DDIV:
        _stack.push(get_top().get_double() / get_top().get_double());
        break;
    case BC_IDIV:
        _stack.push(get_top().get_int() / get_top().get_int());
        break;

    case BC_IMOD:
        _stack.push(get_top().get_int() % get_top().get_int());
        break;

    case BC_IAOR:
        _stack.push(get_top().get_int() | get_top().get_int());
        break;
    case BC_IAAND:
        _stack.push(get_top().get_int() & get_top().get_int());
        break;
    case BC_IAXOR:
        _stack.push(get_top().get_int() ^ get_top().get_int());
        break;

    case BC_DNEG:
    {
        double d = get_top().get_double();
        _stack.push(-d);
    }
        break;
    case BC_INEG:
    {
        int64_t i = get_top().get_int();
        _stack.push(-i);
    }
        break;

    case BC_DPRINT:
        std::cout << get_top().get_double();
        break;
    case BC_IPRINT:
        std::cout << get_top().get_int();
        break;
    case BC_SPRINT:
        std::cout << constantById(get_top().get_str_id());
        break;

    case BC_I2D:
    {
        int64_t i = get_top().get_int();
        _stack.push((double) i);
    }
        break;
    case BC_D2I:
    {
        double d = get_top().get_double();
        _stack.push((int64_t) d);
    }
        break;
    case BC_S2I:
    {
        int16_t u = get_top().get_str_id();
        _stack.push((int64_t) u);
    }
        break;

    case BC_SWAP:
        swap_tops();
        break;
    case BC_POP:
        _stack.pop();
        break;

    case BC_LOADDVAR0:
    case BC_LOADIVAR0:
    case BC_LOADSVAR0:
        _stack.push(_scope->get_var(0));
        break;
    case BC_LOADDVAR1:
    case BC_LOADIVAR1:
    case BC_LOADSVAR1:
        _stack.push(_scope->get_var(1));
        break;
    case BC_LOADDVAR2:
    case BC_LOADIVAR2:
    case BC_LOADSVAR2:
        _stack.push(_scope->get_var(2));
        break;
    case BC_LOADDVAR3:
    case BC_LOADIVAR3:
    case BC_LOADSVAR3:
        _stack.push(_scope->get_var(3));
        break;
    case BC_LOADDVAR:
    case BC_LOADIVAR:
    case BC_LOADSVAR:
    {
        int16_t u = _scope->get_uint16();
        _stack.push(_scope->get_var(u));
    }
        break;

    case BC_STOREIVAR0:
    case BC_STOREDVAR0:
    case BC_STORESVAR0:
        _scope->store_var(get_top(), 0);
        break;
    case BC_STOREIVAR1:
    case BC_STOREDVAR1:
    case BC_STORESVAR1:
        _scope->store_var(get_top(), 1);
        break;
    case BC_STOREIVAR2:
    case BC_STOREDVAR2:
    case BC_STORESVAR2:
        _scope->store_var(get_top(), 2);
        break;
    case BC_STOREIVAR3:
    case BC_STOREDVAR3:
    case BC_STORESVAR3:
        _scope->store_var(get_top(), 3);
        break;
    case BC_STOREIVAR:
    case BC_STOREDVAR:
    case BC_STORESVAR:
    {
        int16_t u = _scope->get_uint16();
        _scope->store_var(get_top(), u);
    }
        break;

    case BC_LOADCTXIVAR:
    case BC_LOADCTXDVAR:
    case BC_LOADCTXSVAR:
    {
        uint16_t scope_id = _scope->get_uint16();
        uint16_t var_id = _scope->get_uint16();
        _stack.push(_scope->get_var(scope_id, var_id));
    }
        break;
    case BC_STORECTXIVAR:
    case BC_STORECTXDVAR:
    case BC_STORECTXSVAR:
    {
        uint16_t scope_id = _scope->get_uint16();
        uint16_t var_id = _scope->get_uint16();
        _scope->store_var(get_top(), scope_id, var_id);
    }
        break;
    case BC_ICMP:
    {
        int64_t l = get_top().get_int();
        int64_t r = get_top().get_int();
        if (l < r) {
            _stack.push((int64_t) -1);
        } else if (l == r) {
            _stack.push((int64_t) 0);
        } else {
            _stack.push((int64_t) 1);
        }
    }
        break;
    case BC_DCMP:
    {
        double l = get_top().get_double();
        double r = get_top().get_double();
        if (l < r) {
            _stack.push((int64_t) -1);
        } else if (l == r) {
            _stack.push((int64_t) 0);
        } else {
            _stack.push((int64_t) 1);
        }
    }
        break;
    case BC_JA:
        _scope->jump();
        break;
    case BC_IFICMPNE:
        _scope->jump_if(get_top().get_int() != get_top().get_int());
        break;
    case BC_IFICMPE:
        _scope->jump_if(get_top().get_int() == get_top().get_int());
        break;
    case BC_IFICMPG:
        _scope->jump_if(get_top().get_int() > get_top().get_int());
        break;
    case BC_IFICMPGE:
        _scope->jump_if(get_top().get_int() >= get_top().get_int());
        break;
    case BC_IFICMPL:
        _scope->jump_if(get_top().get_int() < get_top().get_int());
        break;
    case BC_IFICMPLE:
        _scope->jump_if(get_top().get_int() <= get_top().get_int());
        break;
    case BC_STOP:
        return false;

    case BC_CALL:
    {
        uint16_t func_id = _scope->get_uint16();
        BytecodeFunction* func = (BytecodeFunction*) functionById(func_id);
        _scope = new IScope(func, _scope);
    }
        break;
    case BC_RETURN:
    {
        IScope* cur_scope = _scope;
        _scope = cur_scope->get_parent();
        if (_scope == nullptr) {
            return false;
        }
        delete cur_scope;
    }
        break;

    case BC_BREAK:
    case BC_LAST:
    case BC_INVALID:
        return false;

    case BC_CALLNATIVE:
    default:
        throw std::runtime_error("wrong insn");
    }
    return true;
}





































