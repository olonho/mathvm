#pragma once
#include "mathvm.h"
#include "ast.h"
#include "my_code.h"
#include <map>
#include <string>
#include <stack>
#include <vector>
#include <iostream>

using namespace mathvm;

class Bytecoder : public mathvm::AstVisitor {

	class VarMap {
		typedef uint16_t						id_t;
		typedef map<string, pair<id_t, id_t> >	scope_t;
		typedef vector<scope_t>					all_scopes_t;
		typedef vector<pair<id_t, id_t> >		all_ctx_t;
		typedef map<id_t, id_t>					func_vars_map_t;

	public:
		VarMap(func_vars_map_t* funcVarsCount);
		~VarMap();

		void pushScope(id_t ctx_id);
		void popScope();
		void addDataToScope(std::string const& varName);
		pair<id_t, id_t> getData(std::string const& dataName);

	private:
		all_ctx_t    		contexts_;
		all_scopes_t 		scopes_;
		func_vars_map_t*	funcVarsCount_;
	};

	virtual void visitBinaryOpNode(mathvm::BinaryOpNode* node);
	virtual void visitUnaryOpNode(mathvm::UnaryOpNode* node);
	virtual void visitStringLiteralNode(mathvm::StringLiteralNode* node);
	virtual void visitDoubleLiteralNode(mathvm::DoubleLiteralNode* node);
	virtual void visitIntLiteralNode(mathvm::IntLiteralNode* node);
	virtual void visitLoadNode(mathvm::LoadNode* node);
	virtual void visitStoreNode(mathvm::StoreNode* node);
	virtual void visitBlockNode(mathvm::BlockNode* node);
	virtual void visitNativeCallNode(mathvm::NativeCallNode* node);
	virtual void visitForNode(mathvm::ForNode* node);
	virtual void visitWhileNode(mathvm::WhileNode* node);
	virtual void visitIfNode(mathvm::IfNode* node);
	virtual void visitReturnNode(mathvm::ReturnNode* node);
	virtual void visitFunctionNode(mathvm::FunctionNode* node);
	virtual void visitCallNode(mathvm::CallNode* node);
	virtual void visitPrintNode(mathvm::PrintNode* node);

	// auxiliary
	void placeVar (std::string const& name);
	//void placeFunc(std::string const& name);
	void cmpHandler(TokenKind tokenKind);
	void convertTypes(VarType from_upper, VarType from_lower, VarType to);
	void convertTypes(VarType from, VarType to);

public:
	Bytecoder(Code* code)
        : code_(code),
          bytecode_(((BytecodeFunction *)(code->functionByName(AstFunction::top_name)))->bytecode()),
          topType_(VT_INVALID), returnType_(VT_VOID), ctxId_(0), varMap_(((MyCode*)code_)->getFuncVarsCount()) {
    }

    ~Bytecoder() {}

private:
    Code*		code_;
	Bytecode*	bytecode_;
	VarType		topType_;
	VarType		returnType_;
	uint16_t	ctxId_;
	VarMap		varMap_;
};
