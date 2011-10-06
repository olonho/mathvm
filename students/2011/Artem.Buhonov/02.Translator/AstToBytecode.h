#pragma once
#include "ast.h"
#include <vector>
#include "mathvm.h"

class MyCode : public mathvm::Code 
{
	virtual mathvm::Status * execute(std::vector<mathvm::Var*> vars) { return NULL; }
};

class AstToBytecode : public mathvm::AstVisitor
{
public:
	AstToBytecode() : _currentFreeVarId(0), _lastType(mathvm::VT_INVALID) {}
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
private:
	void insertData(const void *data, size_t size);
	void insertVarId(const std::string &name);
	void throwException(const std::string &what);
	void checkTypeInt(mathvm::AstNode *node);
	void checkIfInsn(mathvm::Instruction insn);
	std::string typeToString(mathvm::VarType type);
	typedef uint8_t VarInt;
	std::map<std::string, std::vector<VarInt> > _vars;
	int _currentFreeVarId;
	mathvm::Bytecode _bytecode;
	mathvm::VarType _lastType;
	MyCode _code;
	static const int VARS_LIMIT = 256;
};

