//
// Created by aleks on 24.11.18.
//

#ifndef VIRTUAL_MACHINES_TYPEINFERENCE_H
#define VIRTUAL_MACHINES_TYPEINFERENCE_H

#include <visitors.h>
#include <memory>

namespace mathvm {
    class Info {
    private:
        VarType type;
    public:
        Info(VarType type) : type(type) {}

        VarType getType() const {
            return type;
        }
    };


    class TypeInferenceVisitor : public AstBaseVisitor {
    public:
        TypeInferenceVisitor(vector <std::unique_ptr<Info>> &ptrs);

        void visitLoadNode(LoadNode *node) override;

        void visitBinaryOpNode(BinaryOpNode *node) override;

        void visitCallNode(CallNode *node) override;

        void visitDoubleLiteralNode(DoubleLiteralNode *node) override;

        void visitStoreNode(StoreNode *node) override;

        void visitStringLiteralNode(StringLiteralNode *node) override;

        void visitIntLiteralNode(IntLiteralNode *node) override;

        void visitUnaryOpNode(UnaryOpNode *node) override;

        void visitNativeCallNode(NativeCallNode *node) override;

        void visitBlockNode(BlockNode *node) override;

        void visitReturnNode(ReturnNode *node) override;

        void visitFunctionNode(FunctionNode *node) override;

    private:
        vector <std::unique_ptr<Info>> &ptrs;
        Scope *currentScope = nullptr;

        Info *createInfo(const VarType& type);
        VarType infer(VarType &left, VarType &right);

    };



}

#endif //VIRTUAL_MACHINES_TYPEINFERENCE_H
