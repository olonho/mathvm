#pragma once

#include <deque>

struct FirstPassVisitor : mathvm::AstVisitor {
  FirstPassVisitor();
  ~FirstPassVisitor();

  virtual void visitUnaryOpNode(mathvm::UnaryOpNode* node);
  virtual void visitBinaryOpNode(mathvm::BinaryOpNode* node);
  virtual void visitStringLiteralNode(mathvm::StringLiteralNode* node);
  virtual void visitDoubleLiteralNode(mathvm::DoubleLiteralNode* node);
  virtual void visitIntLiteralNode(mathvm::IntLiteralNode* node);
  virtual void visitLoadNode(mathvm::LoadNode* node);
  virtual void visitStoreNode(mathvm::StoreNode* node);

  void CheckConversion( std::string const & where, mathvm::VarType fistType, mathvm::VarType secondType, mathvm::AstNode* node );

  virtual void visitForNode(mathvm::ForNode* node);
  virtual void visitWhileNode(mathvm::WhileNode* node);
  virtual void visitIfNode(mathvm::IfNode* node);
  virtual void visitBlockNode(mathvm::BlockNode* node);
  virtual void visitFunctionNode(mathvm::FunctionNode* node);

  void SetNodeType( mathvm::AstNode* node, mathvm::VarType value );
  mathvm::VarType GetNodeType (mathvm::AstNode* node);
  NodeInfo const & GetNodeInfo(mathvm::AstNode* node);

  virtual void visitPrintNode(mathvm::PrintNode* node);
  virtual void visitReturnNode(mathvm::ReturnNode* node);
  virtual void visitCallNode(mathvm::CallNode* node);

  void visit(mathvm::AstFunction * main);
  void VisitFunctions( mathvm::Scope * scope );
  void DeclareFunctions( mathvm::Scope * scope );
  uint16_t GetFunctionId( std::string const & functionName );
  std::deque<mathvm::FunctionNode const*> myFunctions;
  //VariableScopeManager myScopeManager;

  //mathvm::FunctionNode* myCurrentFunction;

private:
  bool myCurrentFunctionBlockNodeVisited;

  ScopeInfo * myCurrentScopeInfo;
  
  typedef std::map<std::string, FunctionID> FunctionDeclarationsMap;
  FunctionDeclarationsMap myFunctionDeclarations;
  std::list<NodeInfo> myNodeInfos;
  std::vector<ScopeInfo*> myScopeInfos;
};