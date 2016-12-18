//
// Created by wimag on 20.11.16.
//

#ifndef MATHVM_CONTEXTSTORAGE_H
#define MATHVM_CONTEXTSTORAGE_H

#include <stdint-gcc.h>
#include <map>
#include <stack>
#include <deque>
#include <ast.h>
#include "mathvm.h"


struct context_entry{
    uint16_t context_id;
    uint16_t var_id;
    bool operator <(const context_entry& other) const;
};

class context{
private:
    std::map<std::string, context_entry> entries;
    uint16_t id;
public:
    context(uint16_t id): id(id){}

    bool has_var(const std::string& name) const;
    context_entry get_var_entry(const std::string& name);
    context_entry add_var(const std::string &name);

    uint16_t size();

    uint16_t getId() const;
};

namespace mathvm{
    class context_storage {
    private:
        std::stack<BytecodeFunction*> functions;
        std::deque<context> contexts;
        std::stack<VarType> types;
    public:
        void enter_function(BytecodeFunction* function);
        void exit_function();
        void top_insn(Instruction insn);
        Bytecode* get_bytecode();
        BytecodeFunction* topmost_function();
        context_entry register_variable(AstVar* astVar);

        VarType tos_type();
        void pop_type();
        void push_type(VarType type);
        VarType top_2_consensus();
        void cast_top_to(VarType type);

        context_entry find_var(string name);
        bool in_tompost_context(context_entry entry);

        context_entry register_variable(const string& name);
    };
}
#endif //MATHVM_CONTEXTSTORAGE_H
