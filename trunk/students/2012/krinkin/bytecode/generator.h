#ifndef __GENERATOR_H__
#define __GENERATOR_H__

#include "bccode.h"
#include "typer.h"

#include "mathvm.h"
#include "ast.h"

#include <map>

using namespace mathvm;

class Generator : public AstVisitor
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

    void translate(AstFunction *top, std::map<AstNode *, VarType> *mapping, BCCode *code);

	Generator() {}
	virtual ~Generator() {}

private:
    std::map<AstNode *, VarType> *m_mapping;
    BCCode *m_code;
    
    void dup_double();
    void dup_int();
    void dup_string();
    void eval_logic(AstNode *node);
    void eval_int(AstNode *node);
    void eval_double(AstNode *node);
    void eval_string(AstNode *node);
    void save_variable(AstVar const * const var);
    void load_variable(AstVar const * const var);
    
    VarType type(AstNode *node) { return (*m_mapping)[node]; }
    Bytecode *bytecode() { return m_code->bytecode(); }
    uint16_t current_id() { return m_code->current_id(); }
    
    void push_bytecode(BytecodeFunction *fun) { m_code->push_bytecode(fun); }
    void pop_bytecode() { m_code->pop_bytecode(); }
    
    void push_scope(Scope *scope) { m_code->push_scope(scope); }
    void pop_scope() { m_code->pop_scope(); }
    
    void visit_scope(Scope *scope);

    AstVar *lookup_variable(std::string const &name)
    {
        return m_code->lookup_variable(name);
    }
    
    std::pair<uint16_t, uint16_t> lookup_variable(AstVar const * const var)
    {
        return m_code->lookup_variable(var);
    }

    AstFunction *lookup_function(std::string const &name)
    {
        return m_code->lookup_function(name);
    }
    
    BytecodeFunction *lookup_function(AstFunction const * const fun)
    {
        return m_code->lookup_function(fun);
    }
    
    std::pair<uint16_t, uint16_t> declare_variable(AstVar *var)
    {
        return m_code->declare_variable(var);
    }
    
    uint16_t declare_function(AstFunction *afun)
    {
        return m_code->declare_function(afun);
    }

    void annotate(uint8_t flag)
    {
        m_code->annotate(current_id(), bytecode()->current(), flag);
    }
};

#endif /* __GENERATOR_H__ */
