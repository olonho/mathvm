/*
 * printer.cpp
 *
 *  Created on: Oct 22, 2012
 *      Author: user
 */

#include "printer.h"

namespace mathvm {

void PrinterVisitor::printAst(AstFunction* top) {
	printBlock(top->node()->body());
}

void PrinterVisitor::visitBinaryOpNode(BinaryOpNode* node) {
	AstNode* left = node->left();
	int currentNodePrecedence = tokenPrecedence(node->kind());
	bool leftNeedsBrackets = (left->isBinaryOpNode()) && (tokenPrecedence(left->asBinaryOpNode()->kind()) < currentNodePrecedence);
	if (leftNeedsBrackets) {
		_out << "(";
	}
	left->visit(this);
	if (leftNeedsBrackets) {
		_out << ")";
	}
	_out << " ";
	_out << tokenOp(node->kind());
	_out << " ";
	AstNode* right = node->right();
	bool rightNeedsBrackets = (right->isBinaryOpNode()) && (tokenPrecedence(right->asBinaryOpNode()->kind()) < currentNodePrecedence);
	if (rightNeedsBrackets) {
		_out << "(";
	}
	right->visit(this);
	if (rightNeedsBrackets)  {
		_out << ")";
	}
}

void PrinterVisitor::visitUnaryOpNode(UnaryOpNode* node) {
	_out << node->kind();
	node->operand()->visit(this);
}

void PrinterVisitor::visitStringLiteralNode(StringLiteralNode* node) {
	const string& literal = node->literal();
	string escapedLiteral;
	unsigned long int literalLength = literal.length();
	for (size_t i = 0; i < literalLength; i++) {
		switch(literal[i]) {
		case '\n':
			escapedLiteral += "\\n";
			break;
		case '\r':
			escapedLiteral += "\\r";
			break;
		case '\t':
			escapedLiteral += "\\t";
			break;
		case '\\':
			escapedLiteral += "\\\\";
			break;
		default:
			escapedLiteral += literal[i];
			break;
		}
	}
	_out << "'" << escapedLiteral << "'";
}

void PrinterVisitor::visitDoubleLiteralNode(DoubleLiteralNode* node) {
	_out << node->literal();
}

void PrinterVisitor::visitIntLiteralNode(IntLiteralNode* node) {
	_out << node->literal();
}

void PrinterVisitor::visitLoadNode(LoadNode* node) {
	_out << node->var()->name();
}

void PrinterVisitor::visitStoreNode(StoreNode* node) {
	_out << node->var()->name() << tokenOp(node->op());
	node->value()->visit(this);
}

void PrinterVisitor::visitForNode(ForNode* node) {
	_out << "for (" << node->var()->name() << " in ";
	node->inExpr()->visit(this);
	_out << ")";
	node->body()->visit(this);
}

void PrinterVisitor::visitWhileNode(WhileNode* node) {
	_out << "while (";
	node->whileExpr()->visit(this);
	_out << ")";
	node->loopBlock()->visit(this);
}

void PrinterVisitor::visitIfNode(IfNode* node) {
	_out << "if (";
	node->ifExpr()->visit(this);
	_out << ")";
	node->elseBlock()->visit(this);
}

void PrinterVisitor::printBlock(BlockNode* node) {
	int nodes_count = node->nodes();
	Scope* scope = node->scope();
	Scope::VarIterator varIt(scope);
	while (varIt.hasNext()) {
		AstVar* var = varIt.next();
		_out << typeToName(var->type()) << " " << var->name() << ";\n";
	}
	Scope::FunctionIterator funcIt(scope);
	while (funcIt.hasNext()) {
		AstFunction* func = funcIt.next();
		func->node()->visit(this);
	}
	for (int i = 0; i < nodes_count; i++) {
		AstNode* currentNode = node->nodeAt(i);
		currentNode->visit(this);
		if (!(currentNode->isIfNode()) && !(currentNode->isBlockNode()) && !(currentNode->isWhileNode()) && !(currentNode->isForNode())) {
			_out << ";";
		}
		_out << "\n";
	}
}

void PrinterVisitor::visitBlockNode(BlockNode* node) {
	_out << "{\n";
	printBlock(node);
	_out << "}\n";
}

void PrinterVisitor::visitFunctionNode(FunctionNode* node) {
	int params_count = node->parametersNumber();
	_out << "function(";
	bool isFirst = true;
	for(int i = 0; i < params_count; i++) {
		if (isFirst) {
			isFirst = false;
		} else {
			_out << ", ";
		}
		_out << node->parameterType(i) << node->parameterName(i);
	}
	_out << ")";
	node->body()->visit(this);
}

void PrinterVisitor::visitReturnNode(ReturnNode* node) {
	cout << "return";
	node->returnExpr()->visit(this);
}

void PrinterVisitor::visitCallNode(CallNode* node) {
	_out << node->name() << "(";
	int params_count = node->parametersNumber();
	bool isFirst = true;
	for (int i = 0; i < params_count; i++) {
		if (isFirst) {
			isFirst = false;
		} else {
			_out << ", ";
		}
		node->parameterAt(i)->visit(this);
	}
	_out << ")";
}

void PrinterVisitor::visitNativeCallNode(NativeCallNode* node) {
	_out << "native '" << node->nativeName() << "';";
}

void PrinterVisitor::visitPrintNode(PrintNode* node) {
	_out << "print(";
	int params_count = node->operands();
	bool isFirst = true;
	for (int i = 0; i < params_count; i++) {
		if (isFirst) {
			isFirst = false;
		} else {
			_out << ", ";
		}
		node->operandAt(i)->visit(this);
	}
	_out << ")";
}
}

