#ifndef VM_IDENTITYTRANSLATOR_H
#define VM_IDENTITYTRANSLATOR_H

#include "../../../../include/mathvm.h"
#include "../../../../include/visitors.h"


namespace mathvm {

    class IdentityTranslator : public Translator {

    public:
        virtual ~IdentityTranslator() override;

        virtual Status *translate(const string &program, Code **code) override;
    };

    const vector<string> operationList = {
#define ENUM_ELEM(t, s, p) s,
            FOR_TOKENS(ENUM_ELEM)
            ""
#undef ENUM_ELEM
    };


    class IdentityVisitor : public AstBaseVisitor {
    private:
        int indentLevel = 0;
        bool shift = false;
        bool softRequest = false;
        static const int indentSize = 4;

        void enterLine();

        void exitLine(bool soft = false);

        void scopePrinter(Scope *scope);

        bool printTop;

        static std::string typeToString(VarType type);

    public:
        IdentityVisitor(bool printTop = true);

        virtual ~IdentityVisitor() override;

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


#endif //VM_IDENTITYTRANSLATOR_H
