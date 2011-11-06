#pragma once
#include <ast.h>
#include <set>
#include <map>
#include <string>
#include <algorithm>
#include <iterator>

class VarsSearcherVisitor : public mathvm::AstVisitor
{
public:
	VarsSearcherVisitor(void);
	~VarsSearcherVisitor(void);
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
	virtual void visitPrintNode(mathvm::PrintNode* node);
	virtual void visitFunctionNode(mathvm::FunctionNode* node);
	virtual void visitCallNode(mathvm::CallNode* node);
	virtual void visitReturnNode(mathvm::ReturnNode* node);
	std::vector<const mathvm::AstVar *> freeVars() {
		std::vector<const mathvm::AstVar *> result;
		std::copy(_freeVars.begin(), _freeVars.end(), std::back_inserter(result));
		return result;
	}
	uint16_t localsCount() const {
		return _localVarsCount;
	}
private:
	void addVar(const mathvm::AstVar *var);

private:
	std::vector<const mathvm::AstVar *> _vars;
	std::set<const mathvm::AstVar *> _freeVars;
	uint16_t _localVarsCount;
};

