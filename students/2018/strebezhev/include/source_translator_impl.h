#ifndef MATHVM_SOURCE_TRANSLATOR_IMPL_H
#define MATHVM_SOURCE_TRANSLATOR_IMPL_H

#include <mathvm.h>
#include <visitors.h>
#include <parser.h>
#include <ast.h>

namespace vm = mathvm;

struct SourceTranslatorImpl : public vm::AstBaseVisitor, vm::Translator {
  unsigned long indent_size = 4;
  unsigned long current_indent = 0;

  SourceTranslatorImpl() = default;

  void visitForNode(vm::ForNode *node) override {
    space("for (");
    print(node->var()->name());
    print(" in ");
    node->inExpr()->visit(this);
    print(") {\n");
    inc();

    node->body()->visit(this);

    dec();
    space("}\n");
  }

  void visitPrintNode(vm::PrintNode *node) override {
    space("print(");

    for (uint32_t i = 0, to = node->operands(); i < to; i++) {
      node->operandAt(i)->visit(this);
      if (i + 1 < to) print(", ");
    }

    print(");");
  }

  void visitLoadNode(vm::LoadNode *node) override {
    print(node->var()->name());
  }

  void visitIfNode(vm::IfNode *node) override {
    space("if (");
    node->ifExpr()->visit(this);
    print(") {\n");
    inc();

    node->thenBlock()->visit(this);

    dec();
    space("}");

    if (node->elseBlock() != nullptr) {
      print(" else {\n");
      inc();

      node->elseBlock()->visit(this);

      dec();
      space("}");
    }

    print("\n");
  }

  void visitBinaryOpNode(vm::BinaryOpNode *node) override {
    print("(");
    node->left()->visit(this);
    print(" ");
    print(node->kind());
    print(" ");
    node->right()->visit(this);
    print(")");
  }

  void visitCallNode(vm::CallNode *node) override {
    print(node->name());
    print("(");

    for (uint32_t i = 0, to = node->parametersNumber(); i < to; i++) {
      node->parameterAt(i)->visit(this);
      if (i + 1 < to) print(", ");
    }

    print(")");
  }

  void visitDoubleLiteralNode(vm::DoubleLiteralNode *node) override {
    print(node->literal());
  }

  void visitStoreNode(vm::StoreNode *node) override {
    space(node->var()->name());
    print(" ");
    print(node->op());
    print(" ");
    node->value()->visit(this);
    print(";");
  }

  void visitStringLiteralNode(vm::StringLiteralNode *node) override {
    print("'");
    printEscaped(node->literal());
    print("'");
  }

  void visitWhileNode(vm::WhileNode *node) override {
    space("while (");
    node->whileExpr()->visit(this);
    print(") {\n");
    inc();

    node->loopBlock()->visit(this);

    dec();
    space("}\n");
  }

  void visitIntLiteralNode(vm::IntLiteralNode *node) override {
    print(node->literal());
  }

  void visitUnaryOpNode(vm::UnaryOpNode *node) override {
    print("(");
    print(node->kind());
    node->operand()->visit(this);
    print(")");
  }

  void visitNativeCallNode(vm::NativeCallNode *node) override {
    print("native '");
    print(node->nativeName());
    print("'");
  }

  void visitBlockNode(vm::BlockNode *node) override {
    vm::Scope::VarIterator it(node->scope());

    while (it.hasNext()) {
      vm::AstVar *var = it.next();
      print(var->type());
      print(" ");
      print(var->name());
      print(";\n");
    }


    vm::Scope::FunctionIterator funIt(node->scope());

    while (funIt.hasNext()) {
      funIt.next()->node()->visit(this);
      print("\n");
    }

    for (uint32_t i = 0; i < node->nodes(); ++i) {
      auto stmt = node->nodeAt(i);
      if (stmt->isCallNode()) space();
      stmt->visit(this);
      if (stmt->isCallNode()) print(";");
      print("\n");
    }
  }

  void visitReturnNode(vm::ReturnNode *node) override {
    space("return");

    if (node->returnExpr()) {
      print(" ");
      node->visitChildren(this);
    }

    print(";");
  }

  void visitFunctionNode(vm::FunctionNode *node) override {
    space("function ");
    print(node->returnType());
    print(" ");
    print(node->name());
    print("(");

    for (uint32_t i = 0, to = node->parametersNumber(); i < to; i++) {
      print(node->parameterType(i));
      print(" ");
      print(node->parameterName(i));
      if (i + 1 < to) print(", ");
    }

    print(") ");

    auto firstChild = node->body()->nodeAt(0);
    if (firstChild->isNativeCallNode()) {
      firstChild->visit(this);
      print(";");
      return;
    }

    print("{\n");
    inc();

    node->body()->visit(this);

    dec();
    space("}\n");
  }

  vm::Status *translate(const std::string &program, vm::Code **code) override {
    vm::Parser p;
    vm::Status *result = p.parseProgram(program);

    if (!result->isOk())
      return result;

    p.top()->node()->body()->visit(this);
    return result;
  }

private:
  void print(const std::string &str) {
    std::cout << str;
  }

  void print(const char *str) {
    std::cout << str;
  }

  void print(char ch) {
    std::cout << ch;
  }

  void print(int64_t i) {
    std::cout << i;
  }

  void print(double i) {
    std::cout << i;
  }

  void print(vm::TokenKind kind) {
    print(vm::tokenOp(kind));
  }

  void print(vm::VarType type) {
    print(typeToName(type));
  }

  void printEscaped(const std::string &str) {
    for (char ch : str) {
      switch (ch) {
        case '\'':
          print("\\'");
          break;
        case '\n':
          print("\\n");
          break;
        case '\r':
          print("\\r");
          break;
        case '\t':
          print("\\t");
          break;
        case '\\':
          print("\\\\");
          break;
        default:
          print(ch);
      }
    }
  }

  void space() {
    print(std::string(current_indent, ' '));
  }

  void space(const std::string &str) {
    space();
    print(str);
  }

  void space(const char *str) {
    space();
    print(str);
  }

  void inc() {
    current_indent += indent_size;
  }

  void dec() {
    current_indent -= indent_size;
  }
};


#endif //MATHVM_SOURCE_TRANSLATOR_IMPL_H
