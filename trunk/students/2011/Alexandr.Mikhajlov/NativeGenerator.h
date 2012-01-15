#pragma once

#include "mathvm.h"
#include "ast.h"
#include "GeneratorCommon.h"
#include "FirstPassVisitor.h"
#include "asmjit\Compiler.h"
#include <asmjit/AsmJit.h>
#include <stack>
	
union AsmVarPtr{
	AsmJit::GPVar *Integer;
	AsmJit::XMMVar *Double;
};

struct MyNativeCode : mathvm::Code {
	virtual mathvm::Status* execute(std::vector<mathvm::Var*>& vars){return NULL;}

	MyNativeCode() : myPtr(NULL) {}

private:
	void * myPtr;

};

struct NativeGenerator : ICodeGenerator, mathvm::AstVisitor {
	virtual void visitUnaryOpNode(mathvm::UnaryOpNode* node) {}
  virtual void visitBinaryOpNode(mathvm::BinaryOpNode* node);
  virtual void visitStringLiteralNode(mathvm::StringLiteralNode* node);
  virtual void visitDoubleLiteralNode(mathvm::DoubleLiteralNode* node);
  virtual void visitIntLiteralNode(mathvm::IntLiteralNode* node);
  virtual void visitLoadNode(mathvm::LoadNode* node){}
  virtual void visitStoreNode(mathvm::StoreNode* node){}
  virtual void visitForNode(mathvm::ForNode* node){}
  virtual void visitWhileNode(mathvm::WhileNode* node){}
  virtual void visitIfNode(mathvm::IfNode* node){}
  virtual void visitBlockNode(mathvm::BlockNode* node);
  virtual void visitFunctionNode(mathvm::FunctionNode* node){}
  virtual void visitPrintNode(mathvm::PrintNode* node);
  virtual void visitReturnNode(mathvm::ReturnNode* node){}
  virtual void visitCallNode(mathvm::CallNode* node){}
  void Compile( mathvm::AstFunction * rootNode);
  virtual mathvm::Code* GetCode()
	{
		return NULL;
	}


private:
  FirstPassVisitor myFirstPassVisitor;
	MyNativeCode myCode;

	AsmJit::Compiler* myCompiler;

	AsmVarPtr myResultVar;

	AsmVarPtr CreateAsmVar(mathvm::VarType type);
	AsmVarPtr VisitWithTypeControl( mathvm::AstNode * node, mathvm::VarType expectedType );
	bool TryDoArithmetics(mathvm::BinaryOpNode* node, AsmVarPtr left, AsmVarPtr right, mathvm::VarType expectedType );
};