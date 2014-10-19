#include "mathvm.h"
#include "parser.h"

#include <iostream>
#include <sstream>

namespace mathvm {

struct AstToStringVisitor : public AstVisitor {
  virtual void visitUnaryOpNode(UnaryOpNode* node) override {
    node->visitChildren(this);
  }
  virtual void visitBinaryOpNode(BinaryOpNode* node) override {
    node->visitChildren(this);
  }
  virtual void visitStringLiteralNode(StringLiteralNode* node) override {
    node->visitChildren(this);
  }
  virtual void visitDoubleLiteralNode(DoubleLiteralNode* node) override {
    node->visitChildren(this);
  }
  virtual void visitIntLiteralNode(IntLiteralNode* node) override {
    node->visitChildren(this);
  }
  virtual void visitLoadNode(LoadNode* node) override {
    node->visitChildren(this);
  }
  virtual void visitStoreNode(StoreNode* node) override {
    node->visitChildren(this);
  }
  virtual void visitForNode(ForNode* node) override {
    node->visitChildren(this);
  }
  virtual void visitWhileNode(WhileNode* node) override {
    node->visitChildren(this);
  }
  virtual void visitIfNode(IfNode* node) override {
    node->visitChildren(this);
  }
  virtual void visitBlockNode(BlockNode* node) override {
    node->visitChildren(this);
  }

  virtual void visitFunctionNode(FunctionNode* node) override {
    ss << typeToName(node->returnType()) << " (";
    for (uint32_t i = 0;; i++) {
      if (i != node->parametersNumber()) {
        ss << typeToName(node->parameterType(i)) << ' ' << node->parameterName(i);
        ss << ", ";
      } else break;
    }
    ss << ") {";
    visitBlockNode(node->body());
    ss << "\n}";
  }

  virtual void visitReturnNode(ReturnNode* node) override {
    node->visitChildren(this);
  }
  virtual void visitCallNode(CallNode* node) override {
    node->visitChildren(this);
  }
  virtual void visitNativeCallNode(NativeCallNode* node) override {
    node->visitChildren(this);
  }
  virtual void visitPrintNode(PrintNode* node) override {
    node->visitChildren(this);
  }

  stringstream ss;
};

struct AstPrinter : public Translator {
  virtual Status *translate(const string &program, Code **code) {
    Parser parser;
    Status *status = parser.parseProgram(program);
    if (status != 0 && status->isError()) return status;
    AstToStringVisitor stringVisitor;
    parser.top()->node()->visit(&stringVisitor);
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