/*
 * SourceByASTPrinter.h
 *
 *  Created on: 28.09.2012
 *      Author: alina
 */

#ifndef SOURCEBYASTPRINTER_H_
#define SOURCEBYASTPRINTER_H_

#include "ast.h"

namespace mathvm {

class SourceByASTPrinter: AstVisitor {
public:
	SourceByASTPrinter();
	virtual ~SourceByASTPrinter();

	void performPrint(AstFunction *top);

private:
	void visitBinaryOpNode(BinaryOpNode *node);
	void visitUnaryOpNode(UnaryOpNode *node);
	void visitStringLiteralNode(StringLiteralNode *node);
	void visitIntLiteralNode(IntLiteralNode *node);
	void visitDoubleLiteralNode(DoubleLiteralNode *node);
	void visitLoadNode(LoadNode *node);
	void visitStoreNode(StoreNode *node);
	void visitBlockNodeWithoutBraces(BlockNode *node);
	void visitBlockNode(BlockNode *node);
	void visitNativeCallNode(NativeCallNode *node);
	void visitForNode(ForNode *node);
	void visitWhileNode(WhileNode *node);
	void visitIfNode(IfNode *node);
	void visitReturnNode(ReturnNode *node);
	void visitFunctionNode(FunctionNode *node);
	void visitCallNode(CallNode *node);
	void visitPrintNode(PrintNode *node);
};

}
#endif /* SOURCEBYASTPRINTER_H_ */
