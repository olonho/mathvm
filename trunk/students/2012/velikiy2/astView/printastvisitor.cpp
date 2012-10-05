#include "printastvisitor.h"

#include <iostream>

using namespace std;


PrintAstVisitor::PrintAstVisitor() : myCurrentIndent(0) {}

void PrintAstVisitor::visitBinaryOpNode(BinaryOpNode* node) {
    node->left()->visit(this);
    cout << " " << tokenOp(node->kind()) << " ";
    node->right()->visit(this);
}
void PrintAstVisitor::visitUnaryOpNode(UnaryOpNode* node) {
    cout << "hello op";
}
void PrintAstVisitor::visitStringLiteralNode(StringLiteralNode* node) {
    cout << "\'str\'";
}
void PrintAstVisitor::visitDoubleLiteralNode(DoubleLiteralNode* node) {
    cout << node->literal();
}
void PrintAstVisitor::visitIntLiteralNode(IntLiteralNode* node) {
    cout << node->literal();
}
void PrintAstVisitor::visitLoadNode(LoadNode* node) {
    cout << node->var()->name();
}
void PrintAstVisitor::visitStoreNode(StoreNode* node) {
    //cout << typeToName(node->var()->type()) << " ";
    cout << node->var()->name() << " = ";
    node->value()->visit(this);
}
void PrintAstVisitor::visitForNode(ForNode* node) {
    cout << "hello for";
}
void PrintAstVisitor::visitWhileNode(WhileNode* node) {
    cout << "hello while";
}
void PrintAstVisitor::visitIfNode(IfNode* node) {
    cout << "hello if";
}
void PrintAstVisitor::visitBlockNode(BlockNode* node) {
    printIndent();
    cout << "{" << endl;
    myCurrentIndent++;
    for(uint32_t i = 0; i < node->nodes(); i++) {
        printIndent();
        node->nodeAt(i)->visit(this);
        cout << ";" << endl;
    }
    myCurrentIndent--;
    cout << "}" << endl;
}
void PrintAstVisitor::visitFunctionNode(FunctionNode* node) {
    printIndent();
    cout << "function " << node->name() << "(" << ")" << endl;
    node->body()->visit(this);
}
void PrintAstVisitor::visitReturnNode(ReturnNode* node) {
    cout << "hello return";
}
void PrintAstVisitor::visitCallNode(CallNode* node) {
    cout << node->name() << "(";
    if(node->parametersNumber() > 0){
        for(uint32_t i = 0; i < node->parametersNumber() - 1; i++) {
            node->parameterAt(i)->visit(this);
            cout << ", ";
        }
        node->parameterAt(node->parametersNumber() - 1)->visit(this);
    }
    cout << ")";
}
void PrintAstVisitor::visitNativeCallNode(NativeCallNode* node) {
    cout << "hello vnative";
}
void PrintAstVisitor::visitPrintNode(PrintNode* node) {
    cout << "print(";
    if(node->operands() > 0){
        for(uint32_t i = 0; i < node->operands() - 1; i++) {
            node->operandAt(i)->visit(this);
            cout << ", ";
        }
        node->operandAt(node->operands() - 1)->visit(this);
    }
    cout << ")";
}

void PrintAstVisitor::printIndent() {
    for (int i = 0; i < myCurrentIndent; i++) {
        cout << "  ";
    }
}

















