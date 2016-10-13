#ifndef AST_PRINTER_H
#define AST_PRINTER_H

#include <string>
#include <iostream>
#include "ast.h"
#include "parser.h"

namespace mathvm {

namespace {

class PrintVisitor : public AstVisitor
{
public:
    PrintVisitor(std::ostream& output);

    void visitBinaryOpNode(BinaryOpNode* node);
    void visitUnaryOpNode(UnaryOpNode* node);
    void visitStringLiteralNode(StringLiteralNode* node);
    void visitDoubleLiteralNode(DoubleLiteralNode* node);
    void visitIntLiteralNode(IntLiteralNode* node);
    void visitLoadNode(LoadNode* node);
    void visitStoreNode(StoreNode* node);
    void visitForNode(ForNode* node);
    void visitWhileNode(WhileNode* node);
    void visitIfNode(IfNode* node);
    void visitBlockNode(BlockNode* node);
    void visitFunctionNode(FunctionNode* node);
    void visitReturnNode(ReturnNode* node);
    void visitCallNode(CallNode* node);
    void visitNativeCallNode(NativeCallNode* node);
    void visitPrintNode(PrintNode* node);

private:
    size_t _nIndent;
    std::ostream& _output;

private:
    std::string indent()
    {
        return std::string(" ", 4 * (_nIndent - 1));
    }
};

} //anonymous namespace

class AstPrinter : public Translator
{
public:
    AstPrinter(std::ostream& output);
    Status* translate(const string& program, Code** code);

private:
    std::ostream& _output;
};

} // mathvm namespace

#endif // AST_PRINTER_H
