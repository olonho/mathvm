#pragma once
#include "ast.h"
#include "GeneratorCommon.h"
#include <deque>
#include "MyInterpreter.h"



struct ByteCodeGenerator : mathvm::AstVisitor
{
  virtual void visitUnaryOpNode(mathvm::UnaryOpNode* node);
  virtual void visitBinaryOpNode(mathvm::BinaryOpNode* node);
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
  virtual void visitReturnNode(mathvm::ReturnNode* node);
  virtual void visitCallNode(mathvm::CallNode* node);
  void Translate( mathvm::AstFunction * rootNode);
  mathvm::Code* GetCode();

  ByteCodeGenerator();
  ~ByteCodeGenerator();

private:
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
  void VisitWithTypeControl(mathvm::AstNode* node, mathvm::VarType expectedType );
  void LoadVarCommand( mathvm::VarType variableType, bool isClosure, VarId const& id );
  void StoreVarCommand( mathvm::VarType variableType, bool isClosure, VarId id );
  
  Interpeter myCode;
  mathvm::Bytecode* myBytecode;
  std::map<mathvm::AstNode const*, mathvm::VarType> myNodeTypes;
  VariableScopeManager myScopeManager;
  mathvm::VarType myLastNodeType;
};

