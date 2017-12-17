#pragma once

#include <visitors.h>
#include "context.h"

namespace mathvm {

    struct BytecodeTranslateVisitor : AstBaseVisitor {

        explicit BytecodeTranslateVisitor(Code* code);

        ~BytecodeTranslateVisitor();

#define VISITOR_FUNCTION(type, name) \
        void visit##type(type* node) override;

        FOR_NODES(VISITOR_FUNCTION)

#undef VISITOR_FUNCTION

        void translate(AstFunction* top);

    private:

        void translateFunction(AstFunction* func);

        void translateLoad(const AstVar* var);

        void translateStore(const AstVar* var);

        Context* getContext();

        Context* findContextByVarName(const string& name);

        void prepareContext(BytecodeFunction* function, Scope* scope = nullptr);

        void clearContext();

        Bytecode* getBytecode();

        VarType getTosType();

        void setTosType(VarType type);

        void castTos(VarType type);

        void visitLogicalOp(BinaryOpNode* node);

        void visitBitwiseOp(BinaryOpNode* node);

        void visitComparingOp(BinaryOpNode* node);

        void visitArithmeticOp(BinaryOpNode* node);

        void visitNotOp(UnaryOpNode* node);

        void visitNegOp(UnaryOpNode* node);

    private:

        Code* _code;
        Context* _currentContext;

    };

}