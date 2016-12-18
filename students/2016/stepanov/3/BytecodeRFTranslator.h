#include "../../../../include/mathvm.h"
#include "../../../../include/visitors.h"
#include "ScopeData.h"


#ifndef VM_BYTECODETRANSLATOR_H
#define VM_BYTECODETRANSLATOR_H


namespace mathvm {

    class BytecodeRFTranslator : public BytecodeTranslatorImpl {

    public:
        virtual ~BytecodeRFTranslator() override;

        virtual Status *translate(const string &program, mathvm::Code **code);
    };

    class BytecodeRfVisitor : private AstVisitor {
    private:
        Code *_code;
        ScopeData *currentSd;
    public:
        BytecodeRfVisitor();

        virtual ~BytecodeRfVisitor() override;

        Status *runTranslate(Code *code, AstFunction *function);

        void translateFunction(AstFunction *function);

        void dumpByteCode(ostream &out);

    private:
        void scopeEvaluator(Scope *scope);

        void storeVariable(const AstVar *var);

        void loadVariable(const AstVar *var);

        void prepareTopType(VarType param);

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

        void processArithmeticOperation(BinaryOpNode *node);

        void processLogicOperation(BinaryOpNode *node);

        void processBitwiseOperation(BinaryOpNode *node);

        void processComparisonOperation(BinaryOpNode *node);

        void loadVariableByInfo(const VariableRF *variableInfo, VarType type);
    };
}


#endif //VM_BYTECODETRANSLATOR_H
