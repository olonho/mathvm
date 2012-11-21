/*
 * printer.h
 *
 *  Created on: Oct 22, 2012
 *      Author: user
 */

#ifndef PRINTER_H_
#define PRINTER_H_

#include "visitors.h"
#include "mathvm.h"
#include "ast.h"

namespace mathvm {

class PrinterVisitor: public AstBaseVisitor {

public:
	PrinterVisitor(std::ostream &outStream): _out(outStream) {}
	virtual ~PrinterVisitor() {}

	void printAst(AstFunction* top);

	virtual void visitBinaryOpNode(BinaryOpNode* node);
	virtual void visitUnaryOpNode(UnaryOpNode* node);
	virtual void visitStringLiteralNode(StringLiteralNode* node);
	virtual void visitDoubleLiteralNode(DoubleLiteralNode* node);
	virtual void visitIntLiteralNode(IntLiteralNode* node);
	virtual void visitLoadNode(LoadNode* node);
	virtual void visitStoreNode(StoreNode* node);
	virtual void visitForNode(ForNode* node);
	virtual void visitWhileNode(WhileNode* node);
	virtual void visitIfNode(IfNode* node);
	virtual void visitBlockNode(BlockNode* node);
	virtual void visitFunctionNode(FunctionNode* node);
	virtual void visitReturnNode(ReturnNode* node);
	virtual void visitCallNode(CallNode* node);
	virtual void visitNativeCallNode(NativeCallNode* node);
	virtual void visitPrintNode(PrintNode* node);

private:
	std::ostream &_out;
	void printBlock(BlockNode* node);

};

}

#endif /* PRINTER_H_ */
