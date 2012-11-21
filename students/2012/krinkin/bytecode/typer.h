#ifndef __TYPER_H__
#define __TYPER_H__

#include <string>
#include <stack>
#include <map>

#include "mathvm.h"
#include "ast.h"

using namespace mathvm;

class Typer : public AstVisitor
{
public:
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

    Status *check(AstFunction *top);
    std::map<AstNode *, VarType> *types() { return m_mapping; }

	Typer() {}
	virtual ~Typer() {}

private:
    std::map<AstNode *, VarType> *m_mapping;
    std::stack<FunctionNode *> m_call_stack;
    std::stack<Scope *> m_scopes_stack;
    Status *m_status;
    
    FunctionNode *function() { return m_call_stack.top(); }
    void push_function(FunctionNode *fun) { m_call_stack.push(fun); }
    void pop_function() { m_call_stack.pop(); }
    
    Scope *scope() { return m_scopes_stack.top(); }
    void push_scope(Scope *scope) { m_scopes_stack.push(scope); }
    void pop_scope() { m_scopes_stack.pop(); }
    
    VarType type(AstNode *node) { return (*m_mapping)[node]; }
    void type(AstNode *node, VarType t) { (*m_mapping)[node] = t; }
    
    bool check_number(AstNode *node)
    {
        if (type(node) == VT_VOID)
        {
            error("VOID is not allowed here", node);
            return false;
        }
        if (type(node) == VT_STRING)
        {
            error("STRING is not allowed here", node);
            return false;
        }
        if (type(node) == VT_INVALID) return false;
        return true;
    }

    bool check_value(AstNode *node)
    {
        if (type(node) == VT_VOID)
        {
            error("VOID is not allowed here", node);
            return false;
        }
        if (type(node) == VT_INVALID) return false;
        return true;
    }
    
    void check_scope();
    
    void error(std::string const &message, AstNode *node)
    {
        if (!m_status) m_status = new Status(message, node->position());
    }
};

#endif /* __TYPER_H__ */

