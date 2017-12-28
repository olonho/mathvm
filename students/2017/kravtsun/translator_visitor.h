#ifndef BYTECODE_TRANSLATOR_H
#define BYTECODE_TRANSLATOR_H

#include <string>
#include <map>
#include "mathvm.h"
#include "ast.h"
#include "visitors.h"
#include "parser.h"
#include "bytecode_impl.h"

namespace mathvm {


class TranslatorVisitor : public AstVisitor {
    // should have some connection to AstVar.
    struct VariableInfo {
        uint16_t func_id, var_id;
        VariableInfo(uint16_t func_id, uint16_t variable_id)
            : func_id(func_id)
            , var_id(variable_id)
        {}
    };
    BytecodeImpl *code_;
    Bytecode *bc_;
    TranslatedFunction *current_function_;

    VarType current_type_;
    Scope *current_scope_;
    uint16_t local_count_;

public:
    TranslatorVisitor(BytecodeImpl *code, Bytecode *bc, TranslatedFunction *top_function);

    ~TranslatorVisitor() final;

    // AstVisitor interface
#define VISITOR_FUNCTION(type, name) \
    void visit##type(type *node) override;
    FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION

private:
    void cast(const VarType &to);

    void numeric_op(const TokenKind &op_type, const VarType &type);
    
    void logical_op(const TokenKind &op_type, const VarType &type);

    void compare_op(const TokenKind &op_type, const VarType &type);

    VariableInfo *resolve(const std::string &name);

    // push to TOS.
    void load_variable(const std::string &name, const VarType &type);

    // pop from TOS.
    void store_variable(const std::string &name, const VarType &type);
    
    vector<VariableInfo*> new_variables;
};

}

#endif // BYTECODE_TRANSLATOR_H

