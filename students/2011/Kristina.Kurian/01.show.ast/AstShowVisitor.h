#pragma once
#include "ast.h"

class AstShowVisitor : public mathvm::AstVisitor {
	virtual void visitBinaryOpNode(mathvm::BinaryOpNode* node);
	virtual void visitUnaryOpNode(mathvm::UnaryOpNode* node);
	virtual void visitStringLiteralNode(mathvm::StringLiteralNode* node);
	virtual void visitDoubleLiteralNode(mathvm::DoubleLiteralNode* node);
	virtual void visitIntLiteralNode(mathvm::IntLiteralNode* node);
	virtual void visitLoadNode(mathvm::LoadNode* node);
	virtual void visitStoreNode(mathvm::StoreNode* node);
	virtual void visitBlockNode(mathvm::BlockNode* node);
	virtual void visitForNode(mathvm::ForNode* node);
	virtual void visitWhileNode(mathvm::WhileNode* node);
	virtual void visitIfNode(mathvm::IfNode* node);
	virtual void visitReturnNode(mathvm::ReturnNode* node); 
	virtual void visitFunctionNode(mathvm::FunctionNode* node);
	virtual void visitCallNode(mathvm::CallNode* node);
	virtual void visitPrintNode(mathvm::PrintNode* node);
public:
	AstShowVisitor();
};
