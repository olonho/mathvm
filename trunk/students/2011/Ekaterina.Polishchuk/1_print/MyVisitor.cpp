#include "MyVisitor.h"

using namespace std;
using namespace mathvm;

void MyVisitor::visitBinaryOpNode(mathvm::BinaryOpNode* node) {
  myStream << "(";
  node->left()->visit(this);
  myStream << tokenOp(node->kind());
  node->right()->visit(this);
  myStream << ")";
}

void MyVisitor::visitUnaryOpNode(mathvm::UnaryOpNode* node) {
  myStream << tokenOp(node->kind());
  node->operand()->visit(this);
}

void MyVisitor::visitStringLiteralNode(mathvm::StringLiteralNode* node) {
  std::string s = node->literal();
  myStream << "\'";
  myStream << "\'";
}

void MyVisitor::visitDoubleLiteralNode(mathvm::DoubleLiteralNode* node) {
  stringstream stream;
  stream << node->literal();
  string s = stream.str();
  int pos = s.find("e+");
  if (pos != -1) {
    s.replace(pos, 2, "e");
  }
  myStream << s;
}

void MyVisitor::visitIntLiteralNode(mathvm::IntLiteralNode* node) {
  myStream << node->literal();
}

void MyVisitor::visitLoadNode(mathvm::LoadNode* node) {
  myStream << node->var()->name(); 
}

void MyVisitor::visitStoreNode(mathvm::StoreNode* node) {
  myStream << node->var()->name() << tokenOp(node->op());
  node->value()->visit(this);
  myStream << ";\n";
}

void MyVisitor::visitForNode(mathvm::ForNode* node) {
  myStream << "for (" << node->var()->name() << " in ";
  node->inExpr()->visit(this);
  myStream << ") {\n";
  node->body()->visit(this);
  myStream << "}\n";
}

void MyVisitor::visitWhileNode(mathvm::WhileNode* node) {
  myStream << "while (";
  node->whileExpr()->visit(this);
  myStream << ") {\n";
  node->loopBlock()->visit(this);
  myStream << "}\n";
}

void MyVisitor::visitIfNode(mathvm::IfNode* node) {
  myStream << "if (";
  node->ifExpr()->visit(this);
  myStream << ") {\n";
  node->thenBlock()->visit(this);
  myStream << "}\n";
  if (node->elseBlock()) {
    myStream << "else {\n";
    node->elseBlock()->visit(this);
    myStream << "}\n";
  }
}

void MyVisitor::visitBlockNode(mathvm::BlockNode* node) {
  mathvm::Scope::VarIterator it(node->scope());
  while (it.hasNext()) {
    AstVar* var = it.next();
    myStream << getTypeName(var->type()) << " " << var->name() << ";\n";
  }
  node->visitChildren(this);
}

void MyVisitor::visitFunctionNode(mathvm::FunctionNode* node) {
  node->visitChildren(this); 
}

void MyVisitor::visitPrintNode(mathvm::PrintNode* node) {
  myStream << "print(";
  node->operands();
  for (unsigned int i = 0; i < node->operands(); ++i) {
    if (i != 0) myStream << ", ";
    AstNode* op = node->operandAt(i);
    op->visit(this);
  }
  myStream << ");\n";
}

std::string MyVisitor::getTypeName(mathvm::VarType type) {
  switch(type) {
    case VT_DOUBLE: 
		return "double";
    case VT_INT: 
		return "int";
    case VT_STRING: 
		return "string";
    case VT_INVALID: 
		return "";
    case VT_VOID: 
		return "void";
  }
  return "";
}