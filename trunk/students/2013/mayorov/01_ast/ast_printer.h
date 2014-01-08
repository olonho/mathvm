#ifndef __AST_PRINTER_H
#define __AST_PRINTER_H

#include <iostream>

#include "ast.h"

using namespace mathvm;

class AstPrinter : public AstVisitor {

public:

	AstPrinter(std::ostream& out) : _out(out) {};
	virtual ~AstPrinter() {};

	void startVisiting(AstFunction* top);

	virtual void visitBinaryOpNode(BinaryOpNode *node);
	virtual void visitUnaryOpNode(UnaryOpNode *node);
	virtual void visitStringLiteralNode(StringLiteralNode *node);
	virtual void visitDoubleLiteralNode(DoubleLiteralNode *node);
	virtual void visitIntLiteralNode(IntLiteralNode *node);
	virtual void visitLoadNode(LoadNode *node);
	virtual void visitStoreNode(StoreNode *node);
	virtual void visitForNode(ForNode *node);
	virtual void visitWhileNode(WhileNode *node);
	virtual void visitIfNode(IfNode *node);
	virtual void visitBlockNode(BlockNode *node);
	virtual void visitFunctionNode(FunctionNode *node);
	virtual void visitReturnNode(ReturnNode *node);
	virtual void visitCallNode(CallNode *node);
	virtual void visitNativeCallNode(NativeCallNode *node);
	virtual void visitPrintNode(PrintNode *node);

private:
	std::ostream& _out;

};


#endif
