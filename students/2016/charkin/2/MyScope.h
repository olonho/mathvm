#pragma once

#include "mathvm.h"
#include "ast.h"

namespace mathvm {
    class MyVar {
    public:
        uint16_t var_id;
        uint16_t var_scope;
        MyVar(uint16_t var_id, uint16_t var_scope)
            : var_id(var_id)
            , var_scope(var_scope)
        {}
    };

    static uint16_t num_of_scopes = 0;

    class MyScope {
    public:
        MyScope(BytecodeFunction* bytecode_function, Scope* scope, MyScope* my_scope = nullptr)
            : _bytecode_function(bytecode_function)
            , _parent(my_scope)
        {
            _scope_id = num_of_scopes++;
            add_vars(Scope::VarIterator(scope));
        }

        bool is_var_exist(std::string name) {
            return (_vars.find(name) != _vars.end()) || (_parent != nullptr && _parent->is_var_exist(name));
        }

        MyVar get_var(std::string name) {
            if (_vars.find(name) != _vars.end()) {
                return MyVar(_vars[name], _scope_id);
            } else {
                if (_parent == nullptr) {
                    throw std::runtime_error("vars doesn't exist: " + name);
                } else {
                    return _parent->get_var(name);
                }
            }
        }

        void add_vars(Scope::VarIterator iter) {
            while (iter.hasNext()) {
                add_var(iter.next());
            }
        }
        void add_var(AstVar* var) {
            _vars[var->name()] = _vars.size();
        }
        BytecodeFunction* get_bytecode_function() {
            return _bytecode_function;
        }
        MyScope* get_parent() {
            return _parent;
        }
        uint16_t get_num_of_vars() {
            return _vars.size();
        }
        uint16_t get_scope_id() {
            return _scope_id;
        }

    private:
        BytecodeFunction* _bytecode_function;
        MyScope* _parent;
        std::map<std::string, uint16_t> _vars;
        uint16_t _scope_id;
    };
}
