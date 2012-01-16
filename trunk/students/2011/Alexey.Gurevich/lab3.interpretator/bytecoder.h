#pragma once
#include "mathvm.h"
#include "ast.h"
#include <map>
#include <string>
#include <stack>
#include <vector>

using namespace mathvm;

class DataMap {
	typedef uint16_t   					   id_t;
	typedef std::map<std::string, id_t >   scope_t;
	typedef std::vector<scope_t>		   all_scopes_t;

public:
	DataMap() {
		scopes_.push_back(scope_t());
		lastVarId_ = 0;
	}

	~DataMap() {
	}

	void pushScope() {
		scopes_.push_back(scope_t());
	}

	void popScope() {
		assert(!scopes_.empty());
		scopes_.pop_back();
	}

	// for variables
	void addDataToScope(std::string const& varName) {
		scopes_.back()[varName] = ++lastVarId_;
	}

	// for functions
	void addDataToScope(std::string const& funcName, id_t id) {
		scopes_.back()[funcName] = id;
	}

	id_t getData(std::string const& dataName) {
		for (all_scopes_t::reverse_iterator it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
			scope_t& curScope = *it;
			if (curScope.count(dataName) != 0)
				return curScope[dataName];
		}
		assert(false);
		return -1;
	}

private:
	all_scopes_t scopes_;
	id_t     	 lastVarId_;
};

class Bytecoder : public mathvm::AstVisitor {
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
	void placeFunc(std::string const& name);
	void cmpHandler(TokenKind tokenKind);
	void convertTypes(VarType from_upper, VarType from_lower, VarType to);
	void convertTypes(VarType from, VarType to);

public:
	Bytecoder(Code* code)
        : code_(code),
          bytecode_(((BytecodeFunction *)(code->functionByName(AstFunction::top_name)))->bytecode()),
          topType_(VT_INVALID), returnType_(VT_VOID), varMap_(), funcMap_() {
    }

    ~Bytecoder() {}

private:
    Code*     code_;
	Bytecode* bytecode_;
	VarType   topType_;
	VarType   returnType_;
	DataMap   varMap_;
	DataMap   funcMap_;
};
