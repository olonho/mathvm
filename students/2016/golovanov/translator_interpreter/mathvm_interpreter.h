#ifndef MATHVM_INTERPRETER_H
#define MATHVM_INTERPRETER_H

#include <unordered_map>
#include <stack>
#include <string>

#include "mathvm.h"

namespace mathvm
{

union val_type
{
    int64_t i;
    double d;
    uint16_t s;

    val_type()
    {
        i = 0;
        d = 0.0;
        s = 0;
    }
};

// Context class
///////////////////////////////////////////////////////////////////////////
class interpreter_context
{
public:
    interpreter_context(BytecodeFunction* top)
        : id_(0)
        , function_(top)
        , parent_(nullptr)
        , ip_(0)
    {}

    BytecodeFunction* function();

    Bytecode* bc();
    uint16_t id();
    interpreter_context* parent();
    interpreter_context* up(BytecodeFunction* func = nullptr);
    interpreter_context* down();
    bool contains(uint16_t var_id);
    void add_var(val_type value);
    void add_var(uint16_t var_id, val_type value);
    void add_var(uint16_t context_id, uint16_t var_id, val_type value);
    val_type get_var(int16_t var_id);
    val_type get_var(uint16_t context_id, uint16_t var_id);

    void move_ip(uint32_t offset);
    uint32_t ip();

private:
    interpreter_context(uint16_t id, BytecodeFunction* func, interpreter_context* parent)
        : id_(id)
        , function_(func)
        , parent_(parent)
        , ip_(0)
    {}

private:
    uint16_t id_;
    BytecodeFunction* function_;
    interpreter_context* parent_;
    uint32_t ip_;
    std::unordered_map<uint16_t, val_type> local_vars_;
};

// Interpreter class
///////////////////////////////////////////////////////////////////////////

class InterpreterCodeImpl : public Code
{
public:
    InterpreterCodeImpl()
        : context_(nullptr)
    {}

    Status* execute(vector<Var*>& vars);

private:
    int64_t get_int();
    double get_double();
    uint16_t get_string();
    uint16_t get_id();
    int16_t get_int16();
    Instruction get_instruction();
    val_type get_var();

    void push(int64_t value);
    void push(double value);
    void push(uint16_t value);

    int64_t pop_int();
    double pop_double();
    uint16_t pop_string();

    bool exec_instruction(Instruction instr);

private:
    interpreter_context* context_;
    std::stack<val_type> stack_;
};

} //mathvm namespace


#endif // MATHVM_INTERPRETER_H
