//
// Created by Mark Geller on 13.10.16.
//

#ifndef MATHVM_TRANSLATOR_IMPL_H
#define MATHVM_TRANSLATOR_IMPL_H

#include <fstream>
#include <iostream>
#include "../../../include/ast.h"
using namespace mathvm;

class ASTPrinter: AstVisitor{
private:

    static const int padding;
    FunctionNode* root;
    int depth_;
    ostream& os;

    void visitBinaryOpNode(BinaryOpNode* node);
    void visitUnaryOpNode(UnaryOpNode *node);
    void visitStringLiteralNode(StringLiteralNode* node);
    void visitIntLiteralNode(IntLiteralNode* node);
    void visitDoubleLiteralNode(DoubleLiteralNode* node);
    void visitLoadNode(LoadNode* node);
    void visitStoreNode(StoreNode* node);
    void visitBlockNode(BlockNode* node);
    void visitNativeCallNode(NativeCallNode* node);
    void visitForNode(ForNode* node);
    void visitIfNode(IfNode* node);
    void visitWhileNode(WhileNode* node);
    void visitReturnNode(ReturnNode* node);
    void visitFunctionNode(FunctionNode* node);
    void visitCallNode(CallNode* node);
    void visitPrintNode(PrintNode* node);
public:
    ASTPrinter(AstFunction* root, ostream& os);
    bool print();
};

class AstVisitor{ };
#endif //MATHVM_TRANSLATOR_IMPL_H
