#include <string>
#include <iostream>
#include <memory>

#include "interpreter_exception.h"
#include "mathvm_interpreter.h"

namespace mathvm
{

// Context class
///////////////////////////////////////////////////////////////////////////
BytecodeFunction* interpreter_context::function()
{
    return function_;
}

Bytecode* interpreter_context::bc()
{
    return function_->bytecode();
}

uint16_t interpreter_context::id()
{
    return id_;
}

interpreter_context* interpreter_context::parent()
{
    return parent_;
}

interpreter_context* interpreter_context::up(BytecodeFunction* func)
{
    if (func == nullptr)
        func = function();

    return new interpreter_context(id() + 1, func, this);
}

interpreter_context* interpreter_context::down()
{
    std::unique_ptr<interpreter_context> bye_bye(this);
    return parent();
}

bool interpreter_context::contains(uint16_t var_id)
{
    return local_vars_.find(var_id) != local_vars_.end() ||
           (parent() != nullptr && parent()->contains(var_id));
}

void interpreter_context::add_var(val_type value)
{
    uint16_t var_id = local_vars_.size();
    local_vars_[var_id] = value;
}

void interpreter_context::add_var(uint16_t var_id, val_type value)
{
    local_vars_[var_id] = value;
}

void interpreter_context::add_var(uint16_t context_id, uint16_t var_id, val_type value)
{
    interpreter_context* current_context = parent();
    while (current_context->id() != context_id)
    {
        current_context = current_context->parent();
    }

    current_context->add_var(var_id, value);
}

val_type interpreter_context::get_var(int16_t var_id)
{
    return local_vars_[var_id];
}

val_type interpreter_context::get_var(uint16_t context_id, uint16_t var_id)
{
    interpreter_context* current_context = parent();
    while (current_context->id() != context_id)
    {
        current_context = current_context->parent();
    }

    return current_context->get_var(var_id);
}

void interpreter_context::move_ip(uint32_t offset)
{
    ip_ += offset;
}

uint32_t interpreter_context::ip()
{
    return ip_;
}


// Interpreter class
///////////////////////////////////////////////////////////////////////////

int64_t InterpreterCodeImpl::get_int()
{
    int64_t result = context_->bc()->getInt64(context_->ip());
    context_->move_ip(sizeof(int64_t));
    return result;
}

double InterpreterCodeImpl::get_double()
{
    double result = context_->bc()->getDouble(context_->ip());
    context_->move_ip(sizeof(double));
    return result;
}

uint16_t InterpreterCodeImpl::get_string()
{
    uint16_t string_id = context_->bc()->getUInt16(context_->ip());
    context_->move_ip(sizeof(uint16_t));
    return string_id;
}

uint16_t InterpreterCodeImpl::get_id()
{
    uint16_t id = context_->bc()->getUInt16(context_->ip());
    context_->move_ip(sizeof(uint16_t));
    return id;
}

val_type InterpreterCodeImpl::get_var()
{
    uint16_t id = context_->bc()->getUInt16(context_->ip());
    context_->move_ip(sizeof(uint16_t));

    return context_->get_var(id);
}

int16_t InterpreterCodeImpl::get_int16()
{
    int16_t v = context_->bc()->getInt16(context_->ip());
    context_->move_ip(sizeof(int16_t));
    return v;
}

Instruction InterpreterCodeImpl::get_instruction()
{
    Instruction instr = context_->bc()->getInsn(context_->ip());
    context_->move_ip(1);
    return instr;
}

void InterpreterCodeImpl::push(int64_t value)
{
    val_type top;
    top.i = value;
    stack_.push(top);
}

void InterpreterCodeImpl::push(double value)
{
    val_type top;
    top.d = value;
    stack_.push(top);
}

void InterpreterCodeImpl::push(uint16_t value)
{
    val_type top;
    top.s = value;
    stack_.push(top);
}

int64_t InterpreterCodeImpl::pop_int()
{
    val_type top = stack_.top();
    stack_.pop();
    return top.i;
}

double InterpreterCodeImpl::pop_double()
{
    val_type top = stack_.top();
    stack_.pop();
    return top.d;
}

uint16_t InterpreterCodeImpl::pop_string()
{
    val_type top = stack_.top();
    stack_.pop();
    return top.s;
}


bool InterpreterCodeImpl::exec_instruction(Instruction instr)
{
    switch(instr)
    {
        case BC_DLOAD:
            push(get_double());
            break;
        case BC_ILOAD:
            push(get_int());
            break;
        case BC_SLOAD:
            push(get_string());
            break;
        case BC_DLOAD0:
            push(0.0);
            break;
        case BC_ILOAD0:
            push((int64_t) 0);
            break;
        case BC_SLOAD0:
            push(makeStringConstant(""));
            break;
        case BC_DLOAD1:
            push(1.0);
            break;
        case BC_ILOAD1:
            push((int64_t) 1);
            break;
        case BC_DLOADM1:
            push(-1.0);
            break;
        case BC_ILOADM1:
            push((int64_t) -1);
            break;
        case BC_DADD:
        {
            double op1 = pop_double();
            double op2 = pop_double();
            push(op1 + op2);
            break;
        };
        case BC_IADD:
        {
            int64_t op1 = pop_int();
            int64_t op2 = pop_int();
            push(op1 + op2);
            break;
        };
        case BC_DSUB:
        {
            double op1 = pop_double();
            double op2 = pop_double();
            push(op1 - op2);
            break;
        };
        case BC_ISUB:
        {
            int64_t op1 = pop_int();
            int64_t op2 = pop_int();
            push(op1 - op2);
            break;
        };
        case BC_DMUL:
        {
            double op1 = pop_double();
            double op2 = pop_double();
            push(op1 * op2);
            break;
        };
        case BC_IMUL:
        {
            int64_t op1 = pop_int();
            int64_t op2 = pop_int();
            push(op1 * op2);
            break;
        };
        case BC_DDIV:
        {
            double op1 = pop_double();
            double op2 = pop_double();
            if (op2 == 0.)
            {
                throw interpreter_exception("Division by zero");
            }
            push(op1 / op2);
            break;
        };
        case BC_IDIV:
        {
            int64_t op1 = pop_int();
            int64_t op2 = pop_int();
            if (op2 == 0)
            {
                throw interpreter_exception("Division by zero");
            }
            push(op1 / op2);
            break;
        };
        case BC_IMOD:
        {
            int64_t op1 = pop_int();
            int64_t op2 = pop_int();
            if (op2 == 0)
            {
                throw interpreter_exception("Division by zero");
            }
            push(op1 % op2);
            break;
        };
        case BC_DNEG:
        {
            double op = pop_double();
            push(-op);
            break;
        };
        case BC_INEG:
        {
            int64_t op = pop_int();
            push(-op);
            break;
        };
        case BC_IAOR:
        {
            int64_t op1 = pop_int();
            int64_t op2 = pop_int();
            push(op1 | op2);
            break;
        };
        case BC_IAAND:
        {
            int64_t op1 = pop_int();
            int64_t op2 = pop_int();
            push(op1 & op2);
            break;
        };
        case BC_IAXOR:
        {
            int64_t op1 = pop_int();
            int64_t op2 = pop_int();
            push(op1 ^ op2);
            break;
        };
        case BC_IPRINT:
        {
            int64_t op = pop_int();
            std::cout << op;
            push(op);
            break;
        };
        case BC_DPRINT:
        {
            double op = pop_double();
            std::cout << op;
            push(op);
            break;
        };
        case BC_SPRINT:
        {
            uint16_t str_id = pop_string();
            std::cout << constantById(str_id);
            push(str_id);
            break;
        };
        case BC_I2D:
        {
            int64_t op = pop_int();
            push((double) op);
            break;
        };
        case BC_D2I:
        {
            double op = pop_double();
            push((double) op);
            break;
        };
        case BC_S2I:
        {
            std::string::size_type sz;
            uint16_t str_id = pop_string();
            int64_t res = std::stoi(constantById(str_id), &sz);
            push(res);
            break;
        };
        case BC_SWAP:
        {
            val_type op1 = stack_.top();
            stack_.pop();
            val_type op2 = stack_.top();
            stack_.pop();
            stack_.push(op1);
            stack_.push(op2);
            break;
        }
        case BC_POP:
        {
            stack_.pop();
            break;
        }
        case BC_LOADIVAR:
        case BC_LOADDVAR:
        case BC_LOADSVAR:
        {
            uint16_t var_id = get_id();
            stack_.push(context_->get_var(var_id));
            break;
        }
        case BC_STOREIVAR:
        case BC_STOREDVAR:
        case BC_STORESVAR:
        {
            uint16_t id = get_id();
            context_->add_var(id, stack_.top());
            stack_.pop();
            break;
        }
        case BC_LOADCTXIVAR:
        case BC_LOADCTXDVAR:
        case BC_LOADCTXSVAR:
        {
            uint16_t context_id = get_id();
            uint16_t var_id = get_id();
            stack_.push(context_->get_var(context_id, var_id));
            break;
        }
        case BC_STORECTXIVAR:
        case BC_STORECTXDVAR:
        case BC_STORECTXSVAR:
        {
            uint16_t context_id = get_id();
            uint16_t var_id = get_id();
            val_type value = stack_.top();
            stack_.pop();
            context_->add_var(context_id, var_id, value);
            break;
        }
        case BC_DCMP:
        {
            double op1 = pop_double();
            double op2 = pop_double();
            if (op1 < op2)
            {
                push(-1.0);
            }
            else if (op1 == op2)
            {
                push(0.0);
            }
            else
            {
                push(1.0);
            }
            break;
        };
        case BC_ICMP:
        {
            int64_t op1 = pop_int();
            int64_t op2 = pop_int();
            if (op1 < op2)
            {
                push((int64_t) -1);
            }
            else if (op1 == op2)
            {
                push((int64_t) 0);
            }
            else
            {
                push((int64_t) 1);
            }
            break;
        };
        case BC_JA:
        {
            int16_t offset = get_int16();
            context_->move_ip(offset - sizeof(int16_t));
            break;
        }
        case BC_IFICMPNE:
        {
            int64_t op1 = pop_int();
            int64_t op2 = pop_int();
            int16_t offset = get_int16();
            if (op1 != op2)
            {
                context_->move_ip(offset - sizeof(int16_t));
            }
            break;
        };
        case BC_IFICMPE:
        {
            int64_t op1 = pop_int();
            int64_t op2 = pop_int();
            int16_t offset = get_int16();
            if (op1 == op2)
            {
                context_->move_ip(offset - sizeof(int16_t));
            }
            break;
        };
        case BC_IFICMPG:
        {
            int64_t op1 = pop_int();
            int64_t op2 = pop_int();
            int16_t offset = get_int16();
            if (op1 > op2)
            {
                context_->move_ip(offset - sizeof(int16_t));
            }
            break;
        };
        case BC_IFICMPGE:
        {
            int64_t op1 = pop_int();
            int64_t op2 = pop_int();
            int16_t offset = get_int16();
            if (op1 >= op2)
            {
                context_->move_ip(offset - sizeof(int16_t));
            }
            break;
        };
        case BC_IFICMPL:
        {
            int64_t op1 = pop_int();
            int64_t op2 = pop_int();
            int16_t offset = get_int16();
            if (op1 < op2)
            {
                context_->move_ip(offset - sizeof(int16_t));
            }
            break;
        };
        case BC_IFICMPLE:
        {
            int64_t op1 = pop_int();
            int64_t op2 = pop_int();
            int16_t offset = get_int16();
            if (op1 <= op2)
            {
                context_->move_ip(offset - sizeof(int16_t));
            }
            break;
        };
        case BC_STOP:
            return false;

        case BC_CALL:
        {
            uint16_t id = get_id();
            BytecodeFunction* func = (BytecodeFunction*) functionById(id);
            context_ = context_->up(func);
            break;
        };
        case BC_RETURN:
        {
            context_ = context_->down();
            break;
        };
        case BC_CALLNATIVE:
            throw interpreter_exception("Native call is unsupported operation");
        default:
            throw interpreter_exception("Invalid instruction ");
    }

    return true;
}

Status* InterpreterCodeImpl::execute(vector<Var*>&)
{
    try
    {
        BytecodeFunction* top = (BytecodeFunction*) functionById(0);
        context_ = new interpreter_context(top);

        while(context_->ip() < context_->bc()->length() &&
              exec_instruction(get_instruction())) {}

        delete context_;

    }
    catch (const interpreter_exception& e)
    {
        return Status::Error(e.what());
    }

    return Status::Ok();
}

} //mathvm namespace
