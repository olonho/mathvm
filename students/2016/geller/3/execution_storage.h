//
// Created by wimag on 23.11.16.
//

#ifndef MATHVM_EXECUTION_STORAGE_H
#define MATHVM_EXECUTION_STORAGE_H

#include <deque>
#include <stack>
#include <mathvm.h>

namespace mathvm{
    class stack_var{
    private:
        VarType type_ = VT_INVALID;
        union {
            int64_t i;
            double d;
            const char* s_ptr;
        } data;
    public:

        stack_var();

        stack_var(int64_t value);

        stack_var(double value);

        stack_var(const char* value);

        explicit operator int64_t() const;

        explicit operator double() const;

        explicit operator const char*() const{ //for some reason it gives error if I put it in cpp
            return data.s_ptr;
        };
        const VarType type();
    };
    std::ostream& operator <<(ostream& stream, stack_var var);

    class stack_frame{
    private:
        vector<stack_var> vars;
        BytecodeFunction* function;
        uint32_t ip;
    public:
        stack_frame(BytecodeFunction* bytecodeFunction);

        uint16_t get_context_id();

        Bytecode* get_bytecode();

        stack_var get_var(uint16_t id);

        void set_var(stack_var var, uint16_t id);

        Instruction next_instruction();

        int64_t load_int();

        double load_double();

        uint16_t load_uint16();

        void jump(int16_t offset);
    };

    class execution_storage {
    private:
        deque<stack_frame> call_stack;
        stack<stack_var> real_stack;
        Code* code;
    public:
        execution_storage(Code * code);

        void enter_function(BytecodeFunction* function);

        stack_var get_local_var(uint16_t id);

        void set_local_var(stack_var var, uint16_t id);

        stack_var get_context_var(uint16_t context_id, uint16_t var_id);

        void set_context_var(stack_var var, uint16_t context_id, uint16_t var_id);

        stack_var pop_stack();

        Instruction next_instruction();

        void exit_function();

        void push_stack(stack_var var);

        bool finished();

        int64_t load_int();

        double load_double();

        const char* load_string();

        uint16_t load_addr();

        void jump(int16_t offset);
    };
}



#endif //MATHVM_EXECUTION_STORAGE_H
