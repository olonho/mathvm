#include "mathvm.h"
#include "ast.h"
#include <iostream>
#include "PresentationVisitor.h"
#include <algorithm>
#include <sstream>

using namespace std;
using namespace mathvm;

void PresentationVisitor::visitBinaryOpNode( mathvm::BinaryOpNode* node )
{
  myStream << "(";
  node->left()->visit(this);
  myStream << tokenOp(node->kind());
  node->right()->visit(this);
  myStream << ")";
}

void PresentationVisitor::visitUnaryOpNode( mathvm::UnaryOpNode* node )
{
  myStream << tokenOp(node->kind());
  node->operand()->visit(this);
}

void PresentationVisitor::visitStringLiteralNode( mathvm::StringLiteralNode* node )
{
  std::string s = node->literal();
  myStream << "\'";
  for_each(s.begin(), s.end(), [this](char c) {
    switch(c) {
      case '\n': myStream << "\\n"; break;
      default: myStream << c;
    }
  });
  myStream << "\'";
}

void PresentationVisitor::visitDoubleLiteralNode( mathvm::DoubleLiteralNode* node )
{
  stringstream stream;
  stream << node->literal();

  string s = stream.str();
  int pos = s.find("e+");
  if (pos != -1) {
    s.replace(pos, 2, "e");
  }
  
  myStream << s;
}

void PresentationVisitor::visitIntLiteralNode( mathvm::IntLiteralNode* node )
{
  myStream << node->literal();
}

void PresentationVisitor::visitLoadNode( mathvm::LoadNode* node )
{
  myStream << node->var()->name(); 
}

void PresentationVisitor::visitStoreNode( mathvm::StoreNode* node )
{
  myStream << node->var()->name() << tokenOp(node->op());
  node->value()->visit(this);
  myStream << ";\n";
}

void PresentationVisitor::visitForNode( mathvm::ForNode* node )
{
  myStream << "for (" << node->var()->name() << " in ";
  node->inExpr()->visit(this);
  myStream << ") {\n";
  node->body()->visit(this);
  myStream << "}\n";
}

void PresentationVisitor::visitWhileNode( mathvm::WhileNode* node )
{
  myStream << "while (";
  node->whileExpr()->visit(this);
  myStream << ") {\n";
  node->loopBlock()->visit(this);
  myStream << "}\n";
}

void PresentationVisitor::visitIfNode( mathvm::IfNode* node )
{
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

void PresentationVisitor::visitBlockNode( mathvm::BlockNode* node )
{
  mathvm::Scope::VarIterator it(node->scope());
  while (it.hasNext()) {
    AstVar* var = it.next();
    myStream << getTypeName(var->type()) << " " << var->name() << ";\n";
  }
  node->visitChildren(this);
}

void PresentationVisitor::visitFunctionNode( mathvm::FunctionNode* node )
{
  node->visitChildren(this); 
}

void PresentationVisitor::visitPrintNode( mathvm::PrintNode* node )
{
  myStream << "print(";
  node->operands();
  for (unsigned int i = 0; i < node->operands(); ++i) {
    if (i != 0) myStream << ", ";
    AstNode* op = node->operandAt(i);
    op->visit(this);
  }
  myStream << ");\n";
}

PresentationVisitor::PresentationVisitor( std::ostream& stream ) : myStream(stream)
{

}

std::string PresentationVisitor::getTypeName( mathvm::VarType type )
{
  switch(type) {
    case VT_DOUBLE: return "double";
    case VT_INT: return "int";
    case VT_STRING: return "string";
    case VT_INVALID: return "";
  }
  return "";
}
