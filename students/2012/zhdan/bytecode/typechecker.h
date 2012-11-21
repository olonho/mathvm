/*
 * typechecker.h
 *
 *  Created on: Oct 27, 2012
 *      Author: user
 */

#ifndef TYPECHECKER_H_
#define TYPECHECKER_H_

#include <stack>

namespace mathvm {

class TypeCheckerVisitor: public AstVisitor {

public:
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
	VarType* getOperationResultType(TokenKind tokenKind, AstNode* left, AstNode* right);
	VarType* getOperationResultType(TokenKind tokenKind, AstNode* operand);
	bool isAssignable(VarType* to, VarType* from);
	VarType* getUpperCommonType(VarType* left, VarType* right);

	Scope* _current_scope;

};

}

#endif /* TYPECHECKER_H_ */

