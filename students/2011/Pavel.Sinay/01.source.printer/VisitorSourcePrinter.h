/*
 * VisitorSourcePrinter.h
 *
 *  Created on: 23.10.2011
 *      Author: niea
 */

#ifndef VISITORSOURCEPRINTER_H_
#define VISITORSOURCEPRINTER_H_

#include "ast.h"
#include "mathvm.h"
#include <iostream>

class VisitorSourcePrinter: public mathvm::AstVisitor {
public:
	VisitorSourcePrinter(std::ostream &msg_stream);
	virtual ~VisitorSourcePrinter();

	virtual void visitBinaryOpNode(mathvm::BinaryOpNode* node);
	virtual void visitUnaryOpNode(mathvm::UnaryOpNode* node);
	virtual void visitStringLiteralNode(mathvm::StringLiteralNode* node);
	virtual void visitDoubleLiteralNode(mathvm::DoubleLiteralNode* node);
	virtual void visitIntLiteralNode(mathvm::IntLiteralNode* node);
	virtual void visitLoadNode(mathvm::LoadNode* node);
	virtual void visitStoreNode(mathvm::StoreNode* node);
	virtual void visitForNode(mathvm::ForNode* node);
	virtual void visitWhileNode(mathvm::WhileNode* node);
	virtual void visitIfNode(mathvm::IfNode* node);
	virtual void visitBlockNode(mathvm::BlockNode* node);
	virtual void visitFunctionNode(mathvm::FunctionNode* node);
	virtual void visitPrintNode(mathvm::PrintNode* node);
	virtual void visitCallNode(mathvm::CallNode* node);
	virtual void visitReturnNode(mathvm::ReturnNode* node);

private:
	std::ostream &m_stream;
	std::string varTypeToStr(mathvm::VarType type);
	void replaceSpecSymbols(std::string &str);
};

#endif /* VISITORSOURCEPRINTER_H_ */
