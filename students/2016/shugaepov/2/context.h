#ifndef MATHVM_CONTEXT_H
#define MATHVM_CONTEXT_H

#include <unordered_map>
#include <string>

#include "../../../../include/mathvm.h"

namespace mathvm {

    class var_context {
    public:
        const uint16_t var_id;
        const uint16_t context_id;

        var_context(uint16_t id, uint16_t context_id)
                : var_id(id)
                , context_id(context_id)
        { }
    };

    class context {

        BytecodeFunction* _bf;
        VarType _tos_type;
        std::unordered_map<std::string, uint16_t> _vars;
        uint16_t _id;

    public:

        context(BytecodeFunction* bf, Scope* scope, uint16_t id)
                : _bf(bf)
        {
            _id = id;

            Scope::VarIterator it = Scope::VarIterator(scope);
            while(it.hasNext())
            {
                AstVar *var = it.next();
                _vars[var->name()] = _vars.size() - 1;
            }

        }

        uint16_t id() { return _id; }

        uint16_t locals_num() { return _vars.size(); }

        BytecodeFunction* byte_func() { return _bf; }


        bool contains_var(std::string const & var_name) { return _vars.find(var_name) != _vars.end(); }

        void add_var(AstVar const * var) { _vars[var->name()] = _vars.size() - 1; }

        var_context v_context(std::string const & var_name) { return var_context(_vars[var_name], _id); }

        VarType& tos_type() { return _tos_type; }
    };

}


#endif //MATHVM_CONTEXT_H
