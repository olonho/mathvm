#include <iostream>
#include <string>

#include "printer_visitor.h"
#include "ast.h"

using namespace std;
using namespace mathvm;

void printVar(const AstVar *var) {
    cout << typeToName(var->type()) << " " << var->name();
}

string getOpString(TokenKind tokenKind) {
    switch (tokenKind) {
    #define ENUM_ELEM(t, s, p) case t: return s;
        FOR_TOKENS(ENUM_ELEM)
    #undef ENUM_ELEM
        default: return "=";
    }
}

void PrintVisitor::visitBlockNode(BlockNode *node) {
    Scope::VarIterator var(node->scope());
    while(var.hasNext()) {
	printVar(var.next());
	cout << ";" << endl;
    }
    Scope::FunctionIterator func(node->scope());
    while(func.hasNext()) func.next()->node()->visit(this);
    node->visitChildren(this);
}

void PrintVisitor::visitForNode(ForNode *node) {
    cout << "for (";
    printVar(node->var());
    cout << " in ";
    node->inExpr()->visit(this);
    cout << ") {" << endl;
    node->body()->visit(this);
    cout << "}" << endl;
}

void PrintVisitor::visitPrintNode(PrintNode *node) {
    cout << "print(";
    for (uint32_t i = 0; i < node->operands(); i++) {
        if (i > 0) cout << ", ";
        AstNode *pNode = node->operandAt(i);
        pNode->visit(this);
    }
    cout << ");" << endl;
}

void PrintVisitor::visitLoadNode(LoadNode *node) {
    cout << node->var()->name();
}

void PrintVisitor::visitIfNode(IfNode *node) {
    cout << "if (";
    node->ifExpr()->visit(this);
    cout << ") {" << endl;
    node->thenBlock()->visit(this);
    cout << "}";
    if (node->elseBlock()) {
        cout << "else {" << endl;
        node->elseBlock()->visit(this);
        cout << "}";
    }
    cout << endl;
}

void PrintVisitor::visitCallNode(CallNode *node) {
    cout << node->name() << "(";
    for (uint32_t i = 0; i < node->parametersNumber(); i++) {
        if (i > 0) cout << ", ";
        node->parameterAt(i)->visit(this);
    }
    cout << ");" << endl;
}

void PrintVisitor::visitDoubleLiteralNode(DoubleLiteralNode *node) {
    cout << node->literal();
}

void PrintVisitor::visitStoreNode(StoreNode *node) {
    cout << node->var()->name() << " " << getOpString(node->op()) << " "; 
    node->value()->visit(this);
    cout << ";" << endl;
}

void PrintVisitor::visitStringLiteralNode(StringLiteralNode *node) {
    cout << "'";
    for (uint32_t i = 0; i < node->literal().length(); ++i)
    {	
	if (node->literal()[i] == '\\') cout << "\\\\";
	else if (node->literal()[i] == '\n') cout << "\\n";
	else if (node->literal()[i] == '\t') cout << "\\t";
	else if (node->literal()[i] == '\r') cout << "\\r";
	else cout << node->literal()[i];
    } 
    cout << "'";
}

void PrintVisitor::visitWhileNode(WhileNode *node) {
    cout << "while (";
    node->whileExpr()->visit(this);
    cout << ") {" << endl;
    node->loopBlock()->visit(this);
    cout << "}" << endl;
}

void PrintVisitor::visitIntLiteralNode(IntLiteralNode *node) {
    cout << node->literal();
}

void PrintVisitor::visitFunctionNode(FunctionNode *node) {
    if(node->name() == "<top>")
    {
	node->body()->visit(this);
	return;
    }
    cout << typeToName(node->returnType()) << " " << node->name() << "(";
    for (uint32_t j = 0; j < node->parametersNumber(); j++) {
        if (j > 0) cout << ", ";
        cout << typeToName(node->parameterType(j)) << " " << node->parameterName(j);
    }
    cout << ") {" << endl;
    node->body()->visit(this);
    cout << "}" << endl;
}

void PrintVisitor::visitBinaryOpNode(BinaryOpNode *node) {
    node->left()->visit(this);
    cout << getOpString(node->kind());
    node->right()->visit(this);
}

void PrintVisitor::visitUnaryOpNode(UnaryOpNode *node) {
    cout << getOpString(node->kind());
    node->operand()->visit(this);
}

void PrintVisitor::visitReturnNode(ReturnNode *node) {
    cout << "return ";
    if(node->returnExpr() != 0) node->returnExpr()->visit(this);
    cout << ";" << endl;
}

void PrintVisitor::visitNativeCallNode(NativeCallNode *node) {
    cout << "native '"<< node->nativeName() << "';" << endl;
}