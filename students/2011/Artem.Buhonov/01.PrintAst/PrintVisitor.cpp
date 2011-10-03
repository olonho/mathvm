#include "PrintVisitor.h"
#include <iostream>
#include <iomanip>

using namespace mathvm;
using namespace std;

void PrintVisitor::visitBinaryOpNode( mathvm::BinaryOpNode* node )
{
	cout << "(";
	node->left()->visit(this);
	cout << " " << tokenOp(node->kind()) << " ";
	node->right()->visit(this);
	cout << ")";
}

void PrintVisitor::visitUnaryOpNode( mathvm::UnaryOpNode* node )
{
	cout << tokenOp(node->kind()) << "(";
	node->visitChildren(this);
	cout << ")";
}

void PrintVisitor::visitStringLiteralNode( mathvm::StringLiteralNode* node )
{
	cout << "\'";
	for(uint32_t i = 0; i < node->literal().size(); ++i) {
		char c = node->literal()[i];
		switch (c) {
		case '\n' : cout << "\\n"; break;
		default: cout << c;
		}	
	};
	cout << "\'";
}

void PrintVisitor::visitDoubleLiteralNode( mathvm::DoubleLiteralNode* node )
{
	cout << node->literal();
}

void PrintVisitor::visitIntLiteralNode( mathvm::IntLiteralNode* node )
{
	cout << node->literal();
}

void PrintVisitor::visitLoadNode( mathvm::LoadNode* node )
{
	cout << node->var()->name();
}

void PrintVisitor::visitStoreNode( mathvm::StoreNode* node )
{
	cout << node->var()->name() << " " << tokenOp(node->op()) << " ";
	node->visitChildren(this);
	cout << ";\n";
}

void PrintVisitor::visitForNode( mathvm::ForNode* node )
{
	cout << "for (";
	cout << node->var()->name() << " in ";
	node->inExpr()->visit(this);	
	cout << ") {\n";
	node->body()->visit(this);
	cout << "\n}\n";
}

void PrintVisitor::visitWhileNode( mathvm::WhileNode* node )
{
	cout << "while (";
	node->whileExpr()->visit(this);
	cout << ") {\n";
	node->loopBlock()->visit(this);
	cout << "\n}\n";
}

void PrintVisitor::visitIfNode( mathvm::IfNode* node )
{
	cout << "if (";
	node->ifExpr()->visit(this);
	cout << ") {\n";
	node->thenBlock()->visit(this);
	cout << "\n}\n";
	if (node->elseBlock()) {
		cout << " else {\n";
		node->elseBlock()->visit(this);
		cout << "\n}\n";
	}
}

void PrintVisitor::visitBlockNode( mathvm::BlockNode* node )
{
	Scope::VarIterator it(node->scope());
	while(it.hasNext()) {
		AstVar *var = it.next();
		string varTypeStr;
		switch (var->type()) {
			case VT_DOUBLE : varTypeStr = "double"; break;
			case VT_INT : varTypeStr = "int"; break;
			case VT_STRING : varTypeStr = "string"; break;
			case VT_INVALID : ;
			case VT_VOID: ;
		}
		cout << varTypeStr << " " << var->name() << ";\n";
	}
	node->visitChildren(this);
}

void PrintVisitor::visitFunctionNode( mathvm::FunctionNode* node )
{
	node->visitChildren(this);
}

void PrintVisitor::visitPrintNode( mathvm::PrintNode* node )
{
	cout << "print(";
	uint32_t count = node->operands();
	for (uint32_t i = 0; i < count; ++i) {
		if (i != 0) {
			cout << ", ";
		}
		node->operandAt(i)->visit(this);
	}
	cout << ");\n";
}
