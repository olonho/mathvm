#ifndef __AST_TRANSLATOR_H__
#define __AST_TRANSLATOR_H__

#include <cassert>
#include <string>
#include <memory>
#include <vector>
#include <stack>

#include "typechecker.h"
#include "mathvm.h"
#include "ast.h"

using namespace mathvm;

class AstTranslator : public AstVisitor
{
public:
	std::auto_ptr<Status> translate(AstFunction *top, Code *code);

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

	AstTranslator() {}
	virtual ~AstTranslator() {}

private:
	std::stack<BytecodeFunction *> m_bytecode_stack;
	std::stack<Scope *> m_scope_stack;
	std::auto_ptr<Status> m_status;
	Code *m_code;
	
	std::map<AstVar const * const,std::pair<uint16_t,uint16_t> > m_variables;
	std::map<AstFunction const * const, BytecodeFunction *> m_functions;
	std::map<std::string, uint16_t> m_natives;
	
	void convert_to_logic(VarType source_type);
	void convert_to(VarType source_type, VarType target_type);
	void dup(VarType source_type);

	void save_var(AstVar const * const var, VarType source_type);
	void load_var(AstVar const * const var);
	
	void load_const(int64_t value);
	void load_const(double value);
	void load_const(std::string const &value);
	
	void call_native(std::string const &name);
	
	void do_comparision(TokenKind kind, VarType source_type);
	void do_binary_operation(TokenKind kind, VarType upper_type, VarType lower_type);
	void do_unary_operation(TokenKind kind, VarType source_type);
	void do_assignment(AstVar const * const var, TokenKind kind, VarType source_type);
	void do_logic(BinaryOpNode const * const node);
	
	void declare_variable(AstVar const * const var, std::pair<uint16_t,uint16_t> id);
	void declare_function(AstFunction *fun);
	void declare_native(const string& name, const Signature& signature);	
	
	Bytecode *bytecode() const;
	BytecodeFunction *function() const;
	uint16_t locals() const;
	void add_local(VarType type);
	Scope *scope() const;
	uint16_t id() const;
	
	void visit_scope(Scope *scope);
	uint16_t size(VarType type) const;
	
	AstVar *lookup_variable(std::string const &name);
	std::pair<uint16_t,uint16_t> lookup_variable(AstVar const * const var);
	AstFunction *lookup_function(std::string const &name);
	uint16_t lookup_function(AstFunction const * const function);
	BytecodeFunction *lookup_bytecode(AstFunction const * const function);
	uint16_t lookup_native(std::string const &name);
};

#endif /* __AST_TRANSLATOR_H__ */
