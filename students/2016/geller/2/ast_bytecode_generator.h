//
// Created by wimag on 20.11.16.
//

#ifndef MATHVM_BYTECODE_GENERATOR_H
#define MATHVM_BYTECODE_GENERATOR_H

#include <ast.h>
#include "mathvm.h"
#include "context_storage.h"
namespace mathvm {

    class ast_bytecode_generator : AstVisitor {
    private:

        AstFunction *root;
        Code* code;
        context_storage storage;

        void visitBinaryOpNode(BinaryOpNode *node) override;
        void visitLazyLogicNode(BinaryOpNode *node);
        void visitArithmeticNode(BinaryOpNode *node);
        void visitComparationNode(BinaryOpNode *node);

        void visitUnaryOpNode(UnaryOpNode *node);
        void visitStringLiteralNode(StringLiteralNode *node);

        void visitIntLiteralNode(IntLiteralNode *node);

        void visitDoubleLiteralNode(DoubleLiteralNode *node);

        void visitLoadNode(LoadNode *node);
        void load_entry(VarType type, context_entry entry);

        void visitStoreNode(StoreNode *node);
        void store_entry(VarType type, context_entry entry);

        void visitBlockNode(BlockNode *node);

//        void visitNativeCallNode(NativeCallNode *node);
//
        void visitForNode(ForNode *node);
//
        void visitIfNode(IfNode *node);
//
        void visitWhileNode(WhileNode *node);
//
        void visitReturnNode(ReturnNode *node);
//
        void visitFunctionNode(FunctionNode *node);
//
        void visitCallNode(CallNode *node);
//
        void visitPrintNode(PrintNode *node);

        void visitAstFunction(AstFunction *function);

    public:
        ast_bytecode_generator(AstFunction* root, Code* code);

        Status* execute();

        virtual ~ast_bytecode_generator() override;
    };
}
#endif //MATHVM_BYTECODE_GENERATOR_H
