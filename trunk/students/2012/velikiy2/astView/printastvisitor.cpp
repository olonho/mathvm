#include "printastvisitor.h"

#include <iostream>

using namespace std;


PrintAstVisitor::PrintAstVisitor() : 
    myCurrentIndent(0), myBlockNoIndentBefore(false), myNoSemiAfter(false) {}

void PrintAstVisitor::visitTopFunction(const AstFunction* rootFunction) {
    
    BlockNode* node = rootFunction->node()->body();
    
    printBlockInner(node);
    
}

void PrintAstVisitor::visitBinaryOpNode(BinaryOpNode* node) {
    node->left()->visit(this);
    cout << " " << tokenOp(node->kind()) << " ";
    node->right()->visit(this);
}
void PrintAstVisitor::visitUnaryOpNode(UnaryOpNode* node) {
    cout << tokenOp(node->kind());
    node->operand()->visit(this);
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
    cout << node->var()->name();
    cout << " " << tokenOp(node->op()) << " ";
    node->value()->visit(this);
}
void PrintAstVisitor::visitForNode(ForNode* node) {
    cout << "for (";
    cout << node->var()->name() << " in ";
    node->inExpr()->visit(this);
    
    cout << ") ";
    myBlockNoIndentBefore = true;
    node->body()->visit(this);
    myBlockNoIndentBefore = false;
    myNoSemiAfter = true;
}
void PrintAstVisitor::visitWhileNode(WhileNode* node) {
    cout << "while (";
    node->whileExpr()->visit(this);
    cout << ") ";
    myBlockNoIndentBefore = true;
    node->loopBlock()->visit(this);
    myBlockNoIndentBefore = false;
    myNoSemiAfter = true;
}
void PrintAstVisitor::visitIfNode(IfNode* node) {
    cout << "if (";
    node->ifExpr()->visit(this);
    cout << ") ";
    myBlockNoIndentBefore = true;
    node->thenBlock()->visit(this);
    myBlockNoIndentBefore = false;
    myNoSemiAfter = true;
    if(node->elseBlock() == 0) return;
    myNoSemiAfter = false;
    cout << "else";
    node->elseBlock()->visit(this);
    myNoSemiAfter = true;
}
void PrintAstVisitor::visitBlockNode(BlockNode* node) {
    if(!myBlockNoIndentBefore)
        printIndent();
    cout << "{" << endl;
    myCurrentIndent++;
    printBlockInner(node);
    myCurrentIndent--;
    printIndent();
    cout << "}" << endl;
}
void PrintAstVisitor::visitFunctionNode(FunctionNode* node) {
    printIndent();
    
    cout << "function " << typeToName(node->returnType()) << " "
         << node->name() << "(";
    if(node->parametersNumber() > 0){
        for(uint32_t i = 0; i < node->parametersNumber(); i++) {
            cout << typeToName(node->parameterType(i)) << " "
                 << node->parameterName(i);
            if(i < node->parametersNumber() - 1)
                cout << ", ";
        }
    }
    cout << ") ";
    node->body()->visit(this);
}
void PrintAstVisitor::visitReturnNode(ReturnNode* node) {
    cout << "return ";
    if(node->returnExpr() != 0)
        node->returnExpr()->visit(this);
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
    if(node->operands() > 0) {
        for(uint32_t i = 0; i < node->operands() - 1; i++) {
            node->operandAt(i)->visit(this);
            cout << ", ";
        }
        node->operandAt(node->operands() - 1)->visit(this);
    }
    cout << ")";
}

void PrintAstVisitor::printBlockInner(BlockNode* node) {
    
    Scope::FunctionIterator funIt(node->scope());
    while(funIt.hasNext()) {
        AstFunction* fun = funIt.next();
        fun->node()->visit(this);
        cout << endl;
    }
    
    Scope::VarIterator varIt(node->scope());
    while(varIt.hasNext()) {
        printIndent();
        AstVar* var = varIt.next();
        cout << typeToName(var->type()) << " " << var->name() << ";" << endl;
    }
    
    for(uint32_t i = 0; i < node->nodes(); i++) {
        printIndent();
        node->nodeAt(i)->visit(this);
        if(myNoSemiAfter)
            myNoSemiAfter = false;
        else
            cout << ";" << endl;
    }
}

void PrintAstVisitor::printIndent() {
    for (int i = 0; i < myCurrentIndent; i++) {
        cout << "  ";
    }
}

















