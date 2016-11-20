#ifndef MATHVM_AST_TO_BYTECODE_H
#define MATHVM_AST_TO_BYTECODE_H

#include "../../../include/ast.h"
#include "../../../include/mathvm.h"
#include "../../../include/visitors.h"

#include "context.h"
#include <deque>
#include <iostream>

using namespace std;

namespace mathvm
{

class ast_to_bytecode_visitor : public AstBaseVisitor
{
private:
    Code* _code;
    deque<context> _context;

    var_context v_context(const string& name)
    {
        for (int i = 0; i < (int)_context.size(); i++)
        {
            if (_context[i].contains_var(name))
                return _context[i].v_context(name);
        }
        throw ;
    }

public:
    ast_to_bytecode_visitor()
        : _code(NULL)
    {
    }

    void translate(Code* code, AstFunction* f);

    void translate_func(AstFunction* f);

    Bytecode* bytecode() { return _context.front().byte_func()->bytecode(); }
    
    VarType tos_type() { return _context.front().tos_type(); }
    
    void set_tos_type(VarType t) { _context.front().tos_type() = t; }

    void load(AstVar const *var);
    void store(AstVar const *var);

    void tos_cast(VarType);
    void arithm_cast(VarType, VarType);

#define VISITOR_FUNCTION_OVERRIDE(type, name) \
    virtual void visit##type(type* node) override;

    FOR_NODES(VISITOR_FUNCTION_OVERRIDE)
#undef VISITOR_FUNCTION_OVERRIDE

};

} // mathvm

#endif //MATHVM_AST_TO_BYTECODE_H
