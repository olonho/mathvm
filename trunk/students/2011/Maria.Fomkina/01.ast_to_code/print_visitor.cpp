#include "print_visitor.h"
#include <cstdio>
#include <cstdlib>

namespace mathvm {
  
std::string types[4] = {"unknown", "double", "int", "string"};
  
char* proc_escape(const char* str) {
  const char* s = str;
  char* new_s = (char*)malloc(1024);
  char* ret = new_s;
  while (*s) {
    switch (*s) {
      case '\'': {*new_s++ = '\\'; *new_s++ = '\''; break;}
      case '\"': {*new_s++ = '\\'; *new_s++ = '\"'; break;}
      case '\\': {*new_s++ = '\\'; *new_s++ = '\\'; break;}
      case '\a': {*new_s++ = '\\'; *new_s++ = 'a'; break;}
      case '\b': {*new_s++ = '\\'; *new_s++ = 'b'; break;}
      case '\f': {*new_s++ = '\\'; *new_s++ = 'f'; break;}
      case '\n': {*new_s++ = '\\'; *new_s++ = 'n'; break;}
      case '\r': {*new_s++ = '\\'; *new_s++ = 'r'; break;}
      case '\t': {*new_s++ = '\\'; *new_s++ = 't'; break;}
      case '\v': {*new_s++ = '\\'; *new_s++ = 'v'; break;}
      default: {*new_s++ = *s;}
    }
    s++;
  }
  *new_s = '\0';
  return ret;
}

void PrintVisitor::visitBinaryOpNode(BinaryOpNode* node) {
  node->left()->visit(this);
  printf(" %s ", tokenOp(node->kind()));
  node->right()->visit(this);
}

void PrintVisitor::visitUnaryOpNode(UnaryOpNode* node) {
  printf("(%s", tokenOp(node->kind()));
  node->operand()->visit(this);
  printf(")");
}

void PrintVisitor::visitStringLiteralNode(StringLiteralNode* node) {
  printf("\'%s\'", proc_escape(node->literal().c_str()));
}

void PrintVisitor::visitDoubleLiteralNode(DoubleLiteralNode* node) {
  printf("%f", node->literal());
}

void PrintVisitor::visitIntLiteralNode(IntLiteralNode* node) {
  printf("%lld", node->literal());
}

void PrintVisitor::visitLoadNode(LoadNode* node) {
  printf("%s", node->var()->name().c_str());
}

void PrintVisitor::visitStoreNode(StoreNode* node) {
  printf("%s %s ", node->var()->name().c_str(), 
         tokenOp(node->op()));
  node->value()->visit(this);
  printf(";\n");
}

void PrintVisitor::visitForNode(ForNode* node) {
  printf("Nothing! ");

}

void PrintVisitor::visitWhileNode(WhileNode* node) {
  printf("Nothing! ");

}

void PrintVisitor::visitIfNode(IfNode* node) {
  printf("Nothing! ");

}

void PrintVisitor::visitBlockNode(BlockNode* node) {
  Scope* scope = node->scope();
  Scope::VarIterator* it = new Scope::VarIterator(scope);
  while (it->hasNext()) {
    AstVar* var = it->next();
    printf("%s %s;\n", types[var->type()].c_str(),
           var->name().c_str());
  }
  node->visitChildren(this);
}

void PrintVisitor::visitFunctionNode(FunctionNode* node) {
  printf("Nothing! ");

}

void PrintVisitor::visitPrintNode(PrintNode* node) {
  printf("print(");
  for (uint32_t i = 0; i < node->operands() - 1; i++) {
    node->operandAt(i)->visit(this);
    printf(", ");
  }
  node->operandAt(node->operands() - 1)->visit(this);
  printf(");\n");
}

}
