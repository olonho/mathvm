#ifndef AST2SRC_VISITOR_H
#define AST2SRC_VISITOR_H

#include <iostream>

#include "visitors.h"
#include "BytecodeHelper.h"

namespace mathvm {

class Ast2BytecodeVisitor : public AstVisitor {
public:
    explicit Ast2BytecodeVisitor(Code* code);

    virtual void visitBinaryOpNode(BinaryOpNode* node);
    virtual void visitUnaryOpNode(UnaryOpNode* node);

    virtual void visitStringLiteralNode(StringLiteralNode* node);
    virtual void visitDoubleLiteralNode(DoubleLiteralNode* node);
    virtual void visitIntLiteralNode(IntLiteralNode* node);

    virtual void visitLoadNode(LoadNode* node);
    virtual void visitStoreNode(StoreNode* node);
    virtual void visitBlockNode(BlockNode* node);

    virtual void visitForNode(ForNode* node);
    virtual void visitWhileNode(WhileNode* node);
    virtual void visitIfNode(IfNode* node);

    virtual void visitFunctionNode(FunctionNode* node);
    virtual void visitReturnNode(ReturnNode* node);
    virtual void visitCallNode(CallNode* node);
    virtual void visitNativeCallNode(NativeCallNode* node);
    virtual void visitPrintNode(PrintNode* node);

    void printBlock(BlockNode* node);
    void start(AstFunction* top);
private:
    Code* _code;
    Bytecode* _bytecode;
    BytecodeHelper _bcHelper;

    BytecodeHelper& bc() { return _bcHelper(_bytecode); }

    void initScope(Scope* scope);
    template <typename TF1, typename TF2>
    void printSignature(const std::string& name, uint32_t size, TF1 at, TF2 action, uint32_t begin = 0) const;

};

}
#endif
