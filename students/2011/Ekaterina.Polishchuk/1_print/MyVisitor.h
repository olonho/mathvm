#ifndef MYVISITOR_H
#define MYVISITOR_H

#include "mathvm.h"
#include "ast.h"
#include <iostream>
#include <algorithm>
#include <sstream>

class MyVisitor : public mathvm::AstVisitor {
private:
  std::ostream & myStream;
  static std::string getTypeName(mathvm::VarType type);
public:  
  MyVisitor(std::ostream& stream): myStream(stream) {}
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
};
#endif
