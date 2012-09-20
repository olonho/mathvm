#ifndef __PRETTY_PRINTER_H__
#define __PRETTY_PRINTER_H__

#include <iterator>
#include <string>

#include "mathvm.h"
#include "ast.h"

using namespace mathvm;

class PrettyPrinter : public AstVisitor
{
public:
	void visitTopLevelBlock(AstFunction const * const top);

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

	PrettyPrinter(std::ostream &out): m_indent(0), m_out(out) {}
	virtual ~PrettyPrinter() {}

private:
	size_t m_indent;
	std::ostream &m_out;

	void printBlock(BlockNode *node);
	void printScope(Scope *scope);

	std::string typeStr(VarType type) const;
	std::string escape(std::string const & str) const;
};

#endif /* __PRETTY_PRINTER_H__ */
