#include "mathvm.h"
#include "parser.h"

#include <iostream>
#include <sstream>
#include <stack>

namespace mathvm {

struct AstToStringVisitor : public AstVisitor {
  virtual void visitUnaryOpNode(UnaryOpNode* node) override {
    st.push(node);
    ss << tokenOp(node->kind()) << '(';
    node->operand()->visit(this);
    ss << ')';
    st.pop();
  }

  virtual void visitBinaryOpNode(BinaryOpNode* node) override {
    st.push(node);
    ss << '(';
    node->left()->visit(this);
    ss << ") " << tokenOp(node->kind()) << " (";
    node->right()->visit(this);
    ss << ')';
    st.pop();
  }

  virtual void visitStringLiteralNode(StringLiteralNode* node) override {
    st.push(node);
    stringstream tmp;
    for (char ch : node->literal()) {
      switch (ch) {
        case '\n': tmp << "\\n"; break;
        default: tmp << ch;
      }
    }
    ss << '\'' << tmp.str() << '\'';
    st.pop();
  }

  virtual void visitDoubleLiteralNode(DoubleLiteralNode* node) override {
    st.push(node);
    ss << node->literal();
    st.pop();
  }

  virtual void visitIntLiteralNode(IntLiteralNode* node) override {
    st.push(node);
    ss << node->literal();
    st.pop();
  }

  virtual void visitLoadNode(LoadNode* node) override {
    st.push(node);
    ss << node->var()->name();
    st.pop();
  }

  virtual void visitStoreNode(StoreNode* node) override {
    st.push(node);
    ss << node->var()->name() << ' ' << tokenOp(node->op()) << ' ';
    node->value()->visit(this);
    ss << ';' << endl;
    st.pop();
  }

  virtual void visitForNode(ForNode* node) override {
    st.push(node);
    ss << "for (" << node->var()->name() << " in ";
    node->inExpr()->visit(this);
    ss << ") ";
    node->body()->visit(this);
    st.pop();
  }

  virtual void visitWhileNode(WhileNode* node) override {
    st.push(node);
    ss << "while (";
    node->whileExpr()->visit(this);
    ss << ") ";
    node->loopBlock()->visit(this);
    st.pop();
  }

  virtual void visitIfNode(IfNode* node) override {
    st.push(node);
    ss << "if (";
    node->ifExpr()->visit(this);
    ss << ") ";
    node->thenBlock()->visit(this);
    if (node->elseBlock() != NULL) {
      ss << " else ";
      node->elseBlock()->visit(this);
    }
    st.pop();
  }

  virtual void visitBlockNode(BlockNode* node) override {
    st.push(node);
    ss << "{" << endl;
    Scope::VarIterator vi = Scope::VarIterator(node->scope());
    while (vi.hasNext()) {
      AstVar *v = vi.next();
      ss << typeToName(v->type()) << ' ' << v->name() << ';' << endl;
    }
    Scope::FunctionIterator fi = Scope::FunctionIterator(node->scope());
    while (fi.hasNext()) {
      fi.next()->node()->visit(this);
    }

    node->visitChildren(this);
    ss << "}" << endl;
    st.pop();
  }

  virtual void visitFunctionNode(FunctionNode* node) override {
    st.push(node);
    ss << "function " << typeToName(node->returnType()) << ' ' << node->name() << " (";
    for (uint32_t i = 0; i < node->parametersNumber(); i++) {
      ss << typeToName(node->parameterType(i)) << ' ' << node->parameterName(i);
      if (i < node->parametersNumber() - 1) ss << ", ";
    }
    ss << ") ";
    node->body()->visit(this);
    st.pop();
  }

  virtual void visitReturnNode(ReturnNode* node) override {
    st.push(node);
    ss << "return ";
    if (node->returnExpr() != NULL)
      node->returnExpr()->visit(this);
    ss << ";" << endl;
    st.pop();
  }

  virtual void visitCallNode(CallNode* node) override {
    st.push(node);
    ss << node->name() << " (";
    for (uint32_t i = 0; i < node->parametersNumber(); i++) {
      node->parameterAt(i)->visit(this);
      if (i < node->parametersNumber() - 1) ss << ", ";
    }
    st.pop();
    if (st.top()->isBlockNode())
      ss << ");" << endl;
    else
      ss << ")";
  }

  virtual void visitNativeCallNode(NativeCallNode* node) override {
    // TODO
  }

  virtual void visitPrintNode(PrintNode* node) override {
    st.push(node);
    ss << "print(";
    for (uint32_t i = 0; i < node->operands(); i++) {
      node->operandAt(i)->visit(this);
      if (i < node->operands() - 1) ss << ", ";
    }
    ss << ");" << endl;
    st.pop();
  }

  std::stack<AstNode*> st;
  stringstream ss;
};

struct AstPrinter : public Translator {
  virtual Status *translate(const string &program, Code **code) {
    Parser parser;
    Status *status = parser.parseProgram(program);
    if (status != 0 && status->isError()) return status;
    AstToStringVisitor stringVisitor;
    parser.top()->node()->body()->visit(&stringVisitor);
    cout << stringVisitor.ss.str() << endl;

    return new Status("No executable code produced");
  }
};

Translator *Translator::create(const string &impl) {
  if (impl == "printer") {
    return new AstPrinter();
  } else {
    return NULL;
  }
}

}