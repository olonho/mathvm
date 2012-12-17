# 1 "check.h"
# 1 "<built-in>"
# 1 "<command-line>"
# 1 "check.h"
# 22 "check.h"
class ASTPrinter : public mathvm::AstVisitor {
public:

    ASTPrinter();
    virtual ~ASTPrinter();




    virtual void visitBinaryOpNode(mathvm::BinaryOpNode* node); virtual void visitUnaryOpNode(mathvm::UnaryOpNode* node); virtual void visitStringLiteralNode(mathvm::StringLiteralNode* node); virtual void visitDoubleLiteralNode(mathvm::DoubleLiteralNode* node); virtual void visitIntLiteralNode(mathvm::IntLiteralNode* node); virtual void visitLoadNode(mathvm::LoadNode* node); virtual void visitStoreNode(mathvm::StoreNode* node); virtual void visitForNode(mathvm::ForNode* node); virtual void visitWhileNode(mathvm::WhileNode* node); virtual void visitIfNode(mathvm::IfNode* node); virtual void visitBlockNode(mathvm::BlockNode* node); virtual void visitFunctionNode(mathvm::FunctionNode* node); virtual void visitReturnNode(mathvm::ReturnNode* node); virtual void visitCallNode(mathvm::CallNode* node); virtual void visitNativeCallNode(mathvm::NativeCallNode* node); virtual void visitPrintNode(mathvm::PrintNode* node);

};
