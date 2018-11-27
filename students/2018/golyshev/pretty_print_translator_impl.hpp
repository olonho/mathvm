#include <mathvm.h>
#include <parser.h>
#include <visitors.h>

#include <memory>
#include <sstream>

    struct PrettyPrintTranslatorImpl : public mathvm::AstBaseVisitor, public mathvm::Translator {
        explicit PrettyPrintTranslatorImpl(std::ostream& buffer, size_t indentSize = 4);

        mathvm::Status* translate(const std::string& program, mathvm::Code** code) override;

        void visitBinaryOpNode(mathvm::BinaryOpNode* node) override;

        void visitUnaryOpNode(mathvm::UnaryOpNode* node) override;

        void visitBlockNode(mathvm::BlockNode* node) override;

        void visitNativeCallNode(mathvm::NativeCallNode* node) override;

        void visitFunctionNode(mathvm::FunctionNode* node) override;

        void visitStoreNode(mathvm::StoreNode* node) override;

        void visitStringLiteralNode(mathvm::StringLiteralNode* node) override;

        void visitDoubleLiteralNode(mathvm::DoubleLiteralNode* node) override;

        void visitIntLiteralNode(mathvm::IntLiteralNode* node) override;

        void visitLoadNode(mathvm::LoadNode* node) override;

        void visitForNode(mathvm::ForNode* node) override;

        void visitWhileNode(mathvm::WhileNode* node) override;

        void visitIfNode(mathvm::IfNode* node) override;

        void visitCallNode(mathvm::CallNode* node) override;

        void visitPrintNode(mathvm::PrintNode* node) override;

        void visitReturnNode(mathvm::ReturnNode* node) override;

    private:
        std::ostream& _buffer;
        const size_t _indentSize;
        size_t _indent;

        void visitTopBlock(mathvm::BlockNode* node);

        void indent();

        void increaseIndent();

        void decreaseIndent();
    };
