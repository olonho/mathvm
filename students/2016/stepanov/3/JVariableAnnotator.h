#ifndef VM_AF_3_JVARIABLEVISITOR_H
#define VM_AF_3_JVARIABLEVISITOR_H


#include <cstring>
#include "../../../../include/mathvm.h"
#include "../../../../include/visitors.h"
#include "../../../../libs/asmjit/asmjit.h"
#include "ScopeData.h"
#include "AScopeData.h"
#include <set>
#include <stack>
#include <queue>
#include <memory>

namespace mathvm {
    class JVariableAnnotator : private AstVisitor {
    private:
        AScopeData *currentSd;
    public:
        void scopeEvaluator(Scope *scope);

        void storeVariable(const AstVar *var);

        void loadVariable(const AstVar *var);

        Status *runTranslate(Code *code, AstFunction *function);

        void translateFunction(AstFunction *function);

        virtual void visitForNode(ForNode *node) override;

        virtual void visitPrintNode(PrintNode *node) override;

        virtual void visitLoadNode(LoadNode *node) override;

        virtual void visitIfNode(IfNode *node) override;

        virtual void visitBinaryOpNode(BinaryOpNode *node) override;

        virtual void visitDoubleLiteralNode(DoubleLiteralNode *node) override;

        virtual void visitStoreNode(StoreNode *node) override;

        virtual void visitStringLiteralNode(StringLiteralNode *node) override;

        virtual void visitWhileNode(WhileNode *node) override;

        virtual void visitIntLiteralNode(IntLiteralNode *node) override;

        virtual void visitUnaryOpNode(UnaryOpNode *node) override;

        virtual void visitNativeCallNode(NativeCallNode *node) override;

        virtual void visitBlockNode(BlockNode *node) override;

        virtual void visitFunctionNode(FunctionNode *node) override;

        virtual void visitReturnNode(ReturnNode *node) override;

        virtual void visitCallNode(CallNode *node) override;

    };

}

#endif //VM_AF_3_JVARIABLEVISITOR_H
