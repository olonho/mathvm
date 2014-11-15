#include "mathvm.h"
#include "parser.h"
#include "visitors.h"

namespace util {

std::string escape(const std::string& s) {
  std::string result;
  for (uint32_t i = 0; i < s.length(); i++) {
    switch (s[i]) {
      case '\n':  result += "\\n";  break;
      case '\r':  result += "\\r";  break;
      case '\t':  result += "\\t";  break;
      case '\'':  result += "\\'";  break;
      case '\\':  result += "\\\\"; break;
      default:    result += s[i];   break;
    }
  }
  return result;
}

}

namespace mathvm {

class AstPrinterVisitor : public AstVisitor {
  ostream &     _output;
  const uint8_t _indentSpaces; // avoid tabs
  uint16_t      _indentLevel;

  void indent() {
    _output << string(_indentSpaces * _indentLevel, ' ');
  }

  void blockEnter() {
    _indentLevel++;
  }

  void blockExit() {
    _indentLevel--;
  }

  void variableDeclaration(Scope* scope) {
    Scope::VarIterator iter(scope);
    while (iter.hasNext()) {
      AstVar* x = iter.next();
      indent();
      _output << typeToName(x->type()) << " "
              << x->name() << ";"
              << endl;
    }
  }

  void functionDeclaration(Scope* scope) {
    Scope::FunctionIterator iter(scope);
    while (iter.hasNext()) {
      indent();
      iter.next()->node()->visit(this);
    }
  }

 public:
  AstPrinterVisitor(ostream & output = std::cout,
                    const uint8_t indentSpaces = 2) :
      _output(output), _indentSpaces(indentSpaces), _indentLevel(0) {
  }

  void insideBlock(BlockNode* node) {
    variableDeclaration(node->scope());
    functionDeclaration(node->scope());

    for (uint32_t i = 0; i < node->nodes(); i++) {
      indent();
      AstNode* current = node->nodeAt(i);
      current->visit(this);
      if (current->isCallNode())
        _output << ";";
      _output << endl;
    }
  }

  virtual void visitBinaryOpNode(BinaryOpNode* node) {
    _output << "(";
    node->left()->visit(this);
    _output << " " << tokenOp(node->kind()) << " ";
    node->right()->visit(this);
    _output << ")";
  }

  virtual void visitUnaryOpNode(UnaryOpNode* node) {
    _output << tokenOp(node->kind());
    node->operand()->visit(this);
  }

  virtual void visitStringLiteralNode(StringLiteralNode* node) {
    _output << "'" << util::escape(node->literal()) << "'";
  }

  virtual void visitDoubleLiteralNode(DoubleLiteralNode* node) {
    _output << node->literal();
  }

  virtual void visitIntLiteralNode(IntLiteralNode* node) {
    _output << node->literal();
  }

  virtual void visitLoadNode(LoadNode* node) {
    _output << node->var()->name();
  }

  virtual void visitStoreNode(StoreNode* node) {
    _output << node->var()->name() << " "
            << tokenOp(node->op()) << " ";
    node->value()->visit(this);
    _output << ";";
  }

  virtual void visitForNode(ForNode* node) {
    _output << "for ("
            << node->var()->name()
            << " in ";
    node->inExpr()->visit(this);
    _output << ")";
    node->body()->visit(this);
  }

  virtual void visitWhileNode(WhileNode* node) {
    _output << "while (";
    node->whileExpr()->visit(this);
    _output << ") ";
    node->loopBlock()->visit(this);
  }

  virtual void visitIfNode(IfNode* node) {
    _output << "if (";
    node->ifExpr()->visit(this);
    _output << ") ";
    node->thenBlock()->visit(this);
    if (node->elseBlock() != NULL) {
      _output << " else ";
      node->elseBlock()->visit(this);
    }
  }

  virtual void visitBlockNode(BlockNode* node) {
    _output << " {" << endl;
    blockEnter();

    insideBlock(node);

    blockExit(); indent();
    _output << "}";
  }

  virtual void visitNativeCallNode(NativeCallNode* node) {
    _output << " native '"
            << node->nativeName()
            << "';" << endl;
  }

  virtual void visitFunctionNode(FunctionNode* node) {
    if (node->name() != AstFunction::top_name) {
      _output << "function "
              << typeToName(node->returnType()) << " "
              << node->name()
              << "(";
      for (uint32_t i = 0; i < node->parametersNumber(); i++) {
        if (i != 0)
          _output << ", ";
        _output << typeToName(node->parameterType(i))
                << " " << node->parameterName(i);
      }
      _output << ")";
    }
    if (node->body()->nodeAt(0)->isNativeCallNode())
      visitNativeCallNode(node->body()->nodeAt(0)->asNativeCallNode());
    else {
      node->body()->visit(this);
      _output << endl;
    }
  }

  virtual void visitReturnNode(ReturnNode* node) {
    _output << "return";
    if (node->returnExpr() != NULL) {
      _output << " ";
      node->returnExpr()->visit(this);
    }
    _output << ";";
  }

  virtual void visitCallNode(CallNode* node) {
    _output << node->name()
            << "(";
    for (uint32_t i = 0; i < node->parametersNumber(); i++) {
      if (i != 0)
        _output << ", ";
      node->parameterAt(i)->visit(this);
    }
    _output << ")";
  }

  virtual void visitPrintNode(PrintNode* node) {
    _output << "print(";
    for (uint32_t i = 0; i < node->operands(); i++) {
      if (i != 0)
        _output << ", ";
      node->operandAt(i)->visit(this);
    }
    _output << ");";
  }

};

class AstPrinter : public Translator {
 public:
  virtual Status* translate(const string& program, Code* *code)  {
    Parser parser;
    Status* status = parser.parseProgram(program);
    if (status->isError()) return status;

    AstPrinterVisitor printer;
    printer.insideBlock(parser.top()->node()->body());

    return Status::Ok();
  }
};

Translator* Translator::create(const string& impl) {
  if (impl == "printer") {
    return new AstPrinter();
  } else {
    return NULL;
  }
}

}
