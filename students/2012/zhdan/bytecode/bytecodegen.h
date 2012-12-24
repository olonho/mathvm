/*
 * codegen.h
 *
 *  Created on: Oct 29, 2012
 *      Author: user
 */

#ifndef CODEGEN_H_
#define CODEGEN_H_

#include "mathvm.h"
#include <string>
#include "astinfo.h"
#include "ast.h"

namespace mathvm {

class CodeGenVisitor: public AstVisitor {

public:
	CodeGenVisitor(Code* code): _code(code){}
	~CodeGenVisitor() {}
	
	virtual BytecodeFunction* generate(AstFunction *top);

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
	Code* _code;
	Scope* _scope;
	std::map<AstFunction*, BytecodeFunction*> _functions;
	std::map<std::string, uint16_t> _variables;
	BytecodeFunction* _current_function;

	Bytecode* getCurrentBytecode();
	void load_string_const(const string& value);
	void load_double_const(double value);
	void load_int_const(int64_t value);
	void load_var(const AstVar* var);
	void process_numbers_bin_op(VarType commonType, AstNode* left, AstNode* right, Instruction ifInt, Instruction ifDouble);
	void process_comprarision(AstNode* left, AstNode* right, Instruction comprassion);
	void process_logic_operation(AstNode* left, AstNode* right, Instruction operation);
	void convert_to_boolean(AstNode* node);
	Scope* get_current_scope();
	void set_current_scope(Scope* scope);
	int get_function_id(AstFunction* function);
	void store(const AstVar* var, VarType type);
	uint16_t get_id() { 
		static int c = 0;
		return c++;	
	}
	void process_scope(Scope* scope);
};

}

#endif /* CODEGEN_H_ */
