#ifndef __TYPE_CHECKER_H__
#define __TYPE_CHECKER_H__

#include "mathvm.h"
#include "ast.h"

#include <memory>

using namespace mathvm;

VarType common(VarType t1, VarType t2);
VarType get_type(CustomDataHolder const * const node);
void set_type(CustomDataHolder * const node, VarType type);

class TypeChecker : public AstVisitor
{
public:
	std::auto_ptr<Status> check(AstFunction *top);

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

	TypeChecker() {}
	virtual ~TypeChecker() {}

private:
	std::auto_ptr<Status> m_status;
	FunctionNode *m_top_function;
	Scope *m_top_scope;
	
	void declare_error(std::string const &str, AstNode const * const node);
	
	bool lvalue(AstNode const * const node) const;
	bool convertable(VarType dst, VarType src) const;
	bool number(VarType type) const;
	
	VarType check_scope(Scope *scope);
};

#endif /* __TYPE_CHECKER_H__ */
