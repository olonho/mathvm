#pragma once

#include <vector>
#include <stack>

#include "mathvm.h"

namespace mathvm {

    class StackItem {
    public:
        StackItem();
        StackItem(int64_t i);
        StackItem(double d);
        StackItem(uint16_t s);
        int64_t get_int();
        double get_double();
        uint16_t get_str_id();
    private:
        union {
            int64_t _int;
            double _double;
            uint16_t _str_id;
        } _item;
        VarType _type;
    };

    class IScope {
    public:
        IScope(BytecodeFunction* bytecode_func, IScope* parent=nullptr);
        Instruction next_insn();
        IScope* get_parent();
        Bytecode* get_bytecode();
        bool has_next_insn();
        int64_t get_int();
        uint16_t get_uint16();
        double get_double();
        StackItem get_var(uint16_t id);
        StackItem get_var(uint16_t scope_id, uint16_t var_id);
        void store_var(StackItem var, uint16_t id);
        void store_var(StackItem var, uint16_t scope_id, uint16_t id);
        void jump();
        void jump_if(bool condition);
        uint32_t IP;
    private:
        BytecodeFunction* _func;
        IScope* _parent;
        std::vector<StackItem> _vars;
    };

    class MyBytecodeInterpretator: public Code {
    public:
        Status *execute(vector<Var *> &vars);
    private:
        IScope* _scope;
        std::stack<StackItem> _stack;
        bool execute_insn(Instruction insn);
        StackItem get_top();
        void swap_tops();
    };
}
