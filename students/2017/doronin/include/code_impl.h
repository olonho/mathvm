#ifndef _CODE_IMPL_H
#define _CODE_IMPL_H

#include <mathvm.h>
#include <map>
#include "mathvm.h"
#include "ast.h"
#include <stack>

namespace mathvm {

class CodeImpl : public Code {
    std::map<Scope*, uint16_t> _scopes;
    std::vector<std::map<const AstVar*, uint16_t>> _scope_vars;
    std::map<const AstVar*, uint16_t> _vars;
    std::vector<BytecodeFunction*> _stack;
    std::map<uint16_t, std::vector<uint16_t>> _dependencies;


public:
    union Data {
        int64_t i;
        double d;
        const char *s;
    };

    std::vector<std::vector<std::vector<Data>>> _memory;



    Data& get_variable(uint16_t scope_id, uint16_t var_id)
    {
        if (_memory[scope_id].size() == 0) {
            _memory[scope_id].emplace_back();
            _memory[scope_id].back().resize(_scope_vars[scope_id].size() + 1);
        }
        return _memory[scope_id].back()[var_id];
    }

    std::vector<Data> _registers;

    size_t _length;
    bool is_stop = false;

    std::stack<Data> _data_stack;
    std::vector<uint32_t> _bci;

    uint32_t& get_bci() {
        return _bci.back();
    }

    uint32_t& get_bci(int idx) {
        return _bci[idx];
    }

    uint32_t get_bci_index() {
        return _bci.size() - 1;
    }

    Bytecode* get_bytecode() {
        return _stack.back()->bytecode();
    }

public:

    void addDependencies(uint16_t function_id, uint16_t scoped_id)
    {
        _dependencies[function_id].push_back(scoped_id);
    }

    uint16_t getScope(Scope *scope) {
        auto it = _scopes.find(scope);
        if (it == _scopes.end()) {
            _scope_vars.emplace_back();
            _scopes[scope] = _scope_vars.size() - 1;
            //std::cout << "scope get " << _scope_vars.size() << std::endl;
            return _scope_vars.size() - 1;
        }

        //std::cout << "scope get " << it->second << std::endl;
        return it->second;
    }

    uint16_t getScopeVar(uint16_t scope, const AstVar *var) {
        //std::cout << "scope var " << scope << std::endl;
        auto it = _scope_vars[scope].find(var);
        if (it == _scope_vars[scope].end()) {
            auto& id = _scope_vars[scope][var];
            id = _scope_vars[scope].size();
            return id;
        }
        //std::cout << "second = " << it->second << std::endl;
        return it->second;
    }

    uint16_t getVar(uint16_t scope, const AstVar *var) {
        auto it = _vars.find(var);
        if (it == _vars.end()) {
            return getScopeVar(scope, var);
        }
        return getScopeVar(it->second, var);
    }

    virtual Status* execute(vector<Var*>& vars) override;

#define INSTRUCTION_FUNCTION(b, d, l) void b();
    FOR_BYTECODES(INSTRUCTION_FUNCTION)
#undef INSTRUCTION_FUNCTION
};

}

#endif // _CODE_IMPL_H

