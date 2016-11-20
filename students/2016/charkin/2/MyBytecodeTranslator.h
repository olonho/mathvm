#pragma once

#include "mathvm.h"
#include "visitors.h"
#include "MyScope.h"

namespace mathvm {
    class MyBytecodeTranslator : public Translator {
    public:
        virtual Status* translate(const string &program, Code **code);
    };

    class MyBytecodeVisitor : public AstVisitor {
    public:
        MyBytecodeVisitor(Code* code);
        Status* translate(AstFunction* root);

#define VISITOR_FUNCTION(type, name) \
    virtual void visit##type(type* node);

    FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION

        void process_function(AstFunction* ast_function, BytecodeFunction* bytecode_function);
        VarType process_logical_op(BinaryOpNode* node);
        VarType process_bitwise_op(BinaryOpNode* node);
        VarType process_comparasion_op(BinaryOpNode* node);
        VarType process_arithmetic_op(BinaryOpNode* node);

    private:
        Code* _code;
        VarType _type;
        MyScope* _scope;
        Bytecode* get_bytecode();
        void cast_type_to(VarType target);
        void store_var(const AstVar* var);
        void load_var(const AstVar* var);
        VarType get_common_type(VarType left_type, VarType right_type);

    };
}
