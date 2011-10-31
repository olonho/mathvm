#pragma once
#include "ast.h"
#include <vector>
#include <map>
#include <string>
#include <sstream>
#include "mathvm.h"
#include "ExecutableCode.h"
#include "Exceptions.h"
#include "FreeVarsFunction.h"

//class MyCode : public mathvm::Code 
//{
//	virtual mathvm::Status * execute(std::vector<mathvm::Var*> &vars) { return NULL; }	
//};

class AstToBytecode : public mathvm::AstVisitor
{
public:
	AstToBytecode(mathvm::Code *code) : _currentFreeVarId(1), _currentFreeFuncId(0), _lastType(mathvm::VT_INVALID), _code(code) {}
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

	void dump() {
		_code->disassemble();
	}

	void generate(mathvm::AstFunction *root) {		
		FreeVarsFunction *mainFunc = new FreeVarsFunction(root);
		_bytecode = mainFunc->bytecode();
		_code->addFunction(mainFunc);
		root->node()->body()->visit(this);
		_bytecode->addInsn(mathvm::BC_STOP);
	}

	mathvm::Bytecode *getBytecode() {
		return _bytecode;
	}
	void pushVar(const std::string &name);
	void popVar(const std::string &name);


private:
	void insertData(const void *data, size_t size);
	void insertVarId(const std::string &name);
	void throwException(const std::string &what);
	void checkCurrentType(mathvm::VarType excpectedType);
	void checkIfInsn(mathvm::Instruction insn);
	std::string typeToString(mathvm::VarType type);
	typedef uint16_t VarInt;
	std::map<std::string, std::vector<VarInt> > _vars;
	int _currentFreeVarId;
	int _currentFreeFuncId;
	mathvm::Bytecode *_bytecode;	
	mathvm::VarType _lastType;
	mathvm::Code *_code;	
	static const int VARS_LIMIT = 256 * 256;
	static const int FUNC_LIMIT = 256 * 256;
};

