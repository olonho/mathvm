#include "printer.h"
#include <iostream>
#include <iomanip>

using namespace mathvm;
using std::cout;
using std::endl;
using std::string;

Printer::Printer()
{
	isSemicolonNeeded = true;
}

string Printer::varTypeToString(mathvm::VarType varType)
{
	string varTypeStr;
	switch (varType) {
		case VT_DOUBLE :  varTypeStr = "double";  break;
		case VT_INT : 	  varTypeStr = "int";     break;
		case VT_STRING :  varTypeStr = "string";  break;
		case VT_VOID:     varTypeStr = "void";    break;
		case VT_INVALID : varTypeStr = "INVALID"; break;
	}
	return varTypeStr;
}

void Printer::visitBinaryOpNode( mathvm::BinaryOpNode* node )
{
	cout << "(";
	bool curIsSemicolonNeeded = isSemicolonNeeded;
	isSemicolonNeeded = false;
	node->left()->visit(this);
	cout << " " << tokenOp(node->kind()) << " ";
	node->right()->visit(this);
	isSemicolonNeeded = curIsSemicolonNeeded;
	cout << ")";
}

void Printer::visitUnaryOpNode( mathvm::UnaryOpNode* node )
{
	cout << "(";
	cout << tokenOp(node->kind());
	bool curIsSemicolonNeeded = isSemicolonNeeded;
	isSemicolonNeeded = false;
	node->visitChildren(this);
	isSemicolonNeeded = curIsSemicolonNeeded;
	cout << ")";
}

void Printer::visitStringLiteralNode( mathvm::StringLiteralNode* node )
{
	cout << "\'";
	for(uint32_t i = 0; i < node->literal().size(); ++i) {
		char c = node->literal()[i];
		switch (c) {
			case '\n' :
				cout << "\\n";
				break;
			default:
				cout << c;
				break;
		}
	};
	cout << "\'";
}

void Printer::visitDoubleLiteralNode( mathvm::DoubleLiteralNode* node )
{
	cout << node->literal();
}

void Printer::visitIntLiteralNode( mathvm::IntLiteralNode* node )
{
	cout << node->literal();
}

void Printer::visitLoadNode( mathvm::LoadNode* node )
{
	cout << node->var()->name();
}

void Printer::visitStoreNode( mathvm::StoreNode* node )
{
	cout << node->var()->name() << " " << tokenOp(node->op()) << " ";
	bool curIsSemicolonNeeded = isSemicolonNeeded;
    isSemicolonNeeded = false;
	node->visitChildren(this);
	isSemicolonNeeded = curIsSemicolonNeeded;
	cout << ";\n";
}

void Printer::visitForNode( mathvm::ForNode* node )
{
	cout << "for (";
	cout << node->var()->name() << " in ";
	bool curIsSemicolonNeeded = isSemicolonNeeded;
	isSemicolonNeeded = false;
	node->inExpr()->visit(this);
	isSemicolonNeeded = curIsSemicolonNeeded;
	cout << ") {\n";
	node->body()->visit(this);
	cout << "}\n";
}

void Printer::visitWhileNode( mathvm::WhileNode* node )
{
	cout << "while (";
	bool curIsSemicolonNeeded = isSemicolonNeeded;
	isSemicolonNeeded = false;
	node->whileExpr()->visit(this);
	isSemicolonNeeded = curIsSemicolonNeeded;
	cout << ") {\n";
	node->loopBlock()->visit(this);
	cout << "}\n";
}

void Printer::visitIfNode( mathvm::IfNode* node )
{
	cout << "if (";
	bool curIsSemicolonNeeded = isSemicolonNeeded;
	isSemicolonNeeded = false;
	node->ifExpr()->visit(this);
	isSemicolonNeeded = curIsSemicolonNeeded;
	cout << ") {\n";
	node->thenBlock()->visit(this);
	cout << "}";
	if (node->elseBlock()) {
		cout << " else {\n";
		node->elseBlock()->visit(this);
		cout << "}\n";
	} else {
		cout << "\n";
	}
}

void Printer::visitBlockNode( mathvm::BlockNode* node )
{
	Scope::FunctionIterator funcIt(node->scope());
	while(funcIt.hasNext()) {
		AstFunction *func = funcIt.next();
		cout << "function " << varTypeToString(func->returnType()) << " " << func->name() << "(";
		uint32_t count = func->parametersNumber();
		for (uint32_t i = 0; i < count; ++i) {
			cout << varTypeToString(func->parameterType(i)) << " " << func->parameterName(i);
			if (i != count - 1)
				cout << ", ";
		}
		cout << ") ";

		// Native functions support:
		if (func->node()->body()->nodes() != 0 && func->node()->body()->nodeAt(0)->isNativeCallNode()) {
			func->node()->body()->nodeAt(0)->visit(this);
		}
		else {
			cout << "{\n";
			func->node()->visit(this);
			cout << "}\n\n";
		}
	}

	Scope::VarIterator it(node->scope());
	while(it.hasNext()) {
		AstVar *var = it.next();
		cout << varTypeToString(var->type()) << " " << var->name() << ";\n";
	}
	node->visitChildren(this);
}

void Printer::visitFunctionNode( mathvm::FunctionNode* node )
{
	node->visitChildren(this);
}

void Printer::visitNativeCallNode( mathvm::NativeCallNode* node )
{
	cout << "native '" << node->nativeName() << "';\n\n";
}

void Printer::visitReturnNode( mathvm::ReturnNode* node )
{
	cout << "return ";
	isSemicolonNeeded = false;
	node->visitChildren(this);
	isSemicolonNeeded = true;
	cout << ";\n";
}

void Printer::visitPrintNode( mathvm::PrintNode* node )
{
	cout << "print(";
	uint32_t count = node->operands();
	isSemicolonNeeded = false;
	for (uint32_t i = 0; i < count; ++i) {
		if (i != 0) {
			cout << ", ";
		}
		node->operandAt(i)->visit(this);
	}
	isSemicolonNeeded = true;
	cout << ");\n";
}

void Printer::visitCallNode( mathvm::CallNode* node )
{	
    cout << node->name() << "(";
	uint32_t count = node->parametersNumber();
	bool curIsSemicolonNeeded = isSemicolonNeeded;
	isSemicolonNeeded = false;
	for (uint32_t i = 0; i < count; ++i) {
		if (i != 0) {
			cout << ", ";
		}
		node->parameterAt(i)->visit(this);
	}
	isSemicolonNeeded = curIsSemicolonNeeded;
	if (isSemicolonNeeded)
		cout << ");\n";
	else
		cout << ")";
}
