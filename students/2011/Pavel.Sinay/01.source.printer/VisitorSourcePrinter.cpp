/*
 * VisitorSourcePrinter.cpp
 *
 *  Created on: 23.10.2011
 *      Author: niea
 */

#include <iostream>
#include "VisitorSourcePrinter.h"

VisitorSourcePrinter::VisitorSourcePrinter() {
	// TODO Auto-generated constructor stub

}

VisitorSourcePrinter::~VisitorSourcePrinter() {
	// TODO Auto-generated destructor stub
}

void VisitorSourcePrinter::visitIfNode(mathvm::IfNode *node) {

}

void VisitorSourcePrinter::visitPrintNode(mathvm::PrintNode *node) {
	std::cout << "print node" << std::endl;
}

void VisitorSourcePrinter::visitLoadNode(mathvm::LoadNode *node) {
}

void VisitorSourcePrinter::visitForNode(mathvm::ForNode *node) {
}

void VisitorSourcePrinter::visitIntLiteralNode(mathvm::IntLiteralNode *node) {
}

void VisitorSourcePrinter::visitUnaryOpNode(mathvm::UnaryOpNode *node) {
}

void VisitorSourcePrinter::visitFunctionNode(mathvm::FunctionNode *node) {
}

void VisitorSourcePrinter::visitWhileNode(mathvm::WhileNode *node) {
}

void VisitorSourcePrinter::visitBinaryOpNode(mathvm::BinaryOpNode *node) {
}

void VisitorSourcePrinter::visitBlockNode(mathvm::BlockNode *node) {
	node->visitChildren(this);
	std::cout << "block node" << std::endl;
}

void VisitorSourcePrinter::visitDoubleLiteralNode(
		mathvm::DoubleLiteralNode *node) {
}

void VisitorSourcePrinter::visitStringLiteralNode(
		mathvm::StringLiteralNode *node) {
}

void VisitorSourcePrinter::visitStoreNode(mathvm::StoreNode *node) {
}

