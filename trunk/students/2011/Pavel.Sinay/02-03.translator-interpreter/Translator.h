/*
 * Translator_.h
 *
 *  Created on: 17.01.2012
 *      Author: Pavel Sinay
 */

#ifndef TRANSLATOR__H_
#define TRANSLATOR__H_

#include "ast.h"
#include "Code.h"
#include "PSVarTable.h"
#include "PSFuncTable.h"

class PSTranslator : public mathvm::AstVisitor {
public:
	PSTranslator();
	virtual ~PSTranslator();
	virtual mathvm::Status* translate(const std::string& program, mathvm::Code* *code);

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
	virtual void visitFunctionNode(mathvm::FunctionNode* node);
	virtual void visitPrintNode(mathvm::PrintNode* node);
	virtual void visitCallNode(mathvm::CallNode* node);
	virtual void visitReturnNode(mathvm::ReturnNode* node);

private:
	mathvm::Code *m_code;
	mathvm::Bytecode m_bytecode;
	PSVarTableTranslate m_var_table;
	PSFuncTable m_func_table;
	mathvm::VarType m_last_result;

	void castIntToDouble();
	void castDoubleToInt();
	void allocVar(mathvm::Var &var);
};

#endif /* TRANSLATOR__H_ */
