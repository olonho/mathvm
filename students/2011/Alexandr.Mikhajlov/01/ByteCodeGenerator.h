#pragma once
#include "ast.h"

struct TranslationException {
  TranslationException(std::string const& message) : myMessage(message) {
  }
    virtual std::string what() const {
    return myMessage;
  }
private:
  std::string myMessage;
};

struct MyCode : mathvm::Code {
  virtual mathvm::Status* execute(std::vector<mathvm::Var*>& vars) {return NULL;}

  void SetMainBytecode(mathvm::Bytecode const & bytecode) {
    myBytecode = bytecode;
  }
  mathvm::Bytecode const & GetMainBytecode() const {
    return myBytecode;
  }

private:
  mathvm::Bytecode myBytecode;
};

struct ByteCodeGenerator : mathvm::AstVisitor
{
	virtual void visitBinaryOpNode(mathvm::BinaryOpNode* node);

  mathvm::VarType DeduceBinaryOperationType( mathvm::VarType leftType, mathvm::VarType rightType);
  
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
  
  void visit( mathvm::AstFunction * rootNode);

  void Dump();
  mathvm::Bytecode* GetBytecode();
  std::vector<std::string> GetStringsVector();

private:

  struct VariableContainer {
    int AddVariable(mathvm::AstVar const * var) {
      // TODO: replace temporary solution
      myVarMap[var] = myVarMap.size();
      return myVarMap.size() - 1;
    }
    uint8_t GetId(mathvm::AstVar const * var) {
      return myVarMap[var];
    }
    bool Exists( mathvm::AstVar const * v ) {
      return myVarMap.find(v) != myVarMap.end();
    }
  private:
    std::map<mathvm::AstVar const *, int> myVarMap;
  };

  void BytecodeAdd(mathvm::VarType expectedType);
  void BytecodeSub(mathvm::VarType expectedType);
  void BytecodeMul(mathvm::VarType expectedType);
  void BytecodeDiv(mathvm::VarType expectedType);
  void BytecodePrint(mathvm::VarType expectedType);
  void BytecodeNeg(mathvm::VarType expectedType);

  bool TryDoArithmetics(mathvm::BinaryOpNode * node, mathvm::VarType expectedType);
  bool TryDoIntegerLogic( mathvm::BinaryOpNode*  node );
  bool TryDoFloatingLogic( mathvm::BinaryOpNode* node );
  void DoIFICMP(mathvm::Instruction operation );
  void LoadVar( mathvm::AstVar const * var );
  void StoreVar( mathvm::AstVar const * var );

  
  MyCode myCode;
  VariableContainer myVariables;
  mathvm::Bytecode myBytecode;
  std::map<mathvm::AstNode const*, mathvm::VarType> myNodeTypes;
};

