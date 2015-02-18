#include "mathvm.h"
#include "parser.h"
#include "visitors.h"
#include "CppCode.h"
#include <unistd.h>
#include <fstream>

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

std::string typeToCppType(VarType type) {
  switch (type) {
  case VT_INT:    return "int64_t";
  case VT_DOUBLE: return "double";
  case VT_STRING: return "const char*";
  case VT_VOID:   return "void";
  default: assert(false);
  }
}

bool isNativeNode(FunctionNode* node) {
  return node->body()->nodes() > 0
      && node->body()->nodeAt(0)->isNativeCallNode();
}

class NativeVisitor : public AstVisitor {
  ostream &     _output;
  const uint8_t _indentSpaces;

  void indent() {
    _output << string(_indentSpaces, ' ');
  }

  void visitFunctions(Scope* scope) {
    Scope::FunctionIterator iter(scope);
    while (iter.hasNext()) {
      iter.next()->node()->visit(this);
    }
  }

public:
  NativeVisitor(ostream & output = std::cout,
                const uint8_t indentSpaces = 2) :
      _output(output), _indentSpaces(indentSpaces) {
  }

  virtual void visitFunctionNode(FunctionNode* node) {
    if (!isNativeNode(node)) {
      node->body()->visit(this);
    } else {
      indent();
      _output << typeToCppType(node->returnType())
              << " (* native_"
              << node->body()->nodeAt(0)->asNativeCallNode()->nativeName();
      _output << ")(";
      for (uint32_t i = 0; i < node->parametersNumber(); i++) {
        if (i != 0)
            _output << ", ";
        _output << typeToCppType(node->parameterType(i));
      }
      _output << ") = (";
      _output << typeToCppType(node->returnType())
              << " (*)(";
      for (uint32_t i = 0; i < node->parametersNumber(); i++) {
        if (i != 0)
            _output << ", ";
        _output << typeToCppType(node->parameterType(i));
      }
      _output << ")) dlsym(RTLD_DEFAULT, \""
              << node->body()->nodeAt(0)->asNativeCallNode()->nativeName()
              << "\");\n";
      indent();
      _output << "assert(native_"
              << node->body()->nodeAt(0)->asNativeCallNode()->nativeName()
              << ");\n";

    }
  }

  virtual void visitBlockNode(BlockNode* node) {
    visitFunctions(node->scope());
  }
};

class ToCpp11Visitor : public AstVisitor {
  ostream &     _output;
  const uint8_t _indentSpaces; // avoid tabs
  uint16_t      _indentLevel;
  Scope*        _scope;
  FunctionNode* _function;

  void indent() {
    _output << string(_indentSpaces * _indentLevel, ' ');
  }

  void blockEnter() {
    _indentLevel++;
  }

  void blockExit() {
    _indentLevel--;
  }

  void variableDeclarations(Scope* scope) {
    Scope::VarIterator iter(scope);
    while (iter.hasNext()) {
      AstVar* x = iter.next();
      indent();
      _output << typeToCppType(x->type()) << " "
              << x->name() << ";\n";
    }
  }

  void functionDeclarations(Scope* scope) {
    Scope::FunctionIterator iter(scope);
    while (iter.hasNext()) {
      FunctionNode* node = iter.next()->node();
      if (isNativeNode(node))
        continue;
      indent();
      functionSignature(node);
      _output << "function_" << node->name() << ";\n";
    }
  }

  void functionSignature(FunctionNode* node) {
    if (node->name() != AstFunction::top_name) {
      _output << "std::function<"
              << typeToCppType(node->returnType())
              << "(";
      for (uint32_t i = 0; i < node->parametersNumber(); i++) {
        if (i != 0)
          _output << ", ";
        _output << typeToCppType(node->parameterType(i));
      }
      _output << ")> ";
    }
  }

  void visitFunctions(Scope* scope) {
    Scope::FunctionIterator iter(scope);
    while (iter.hasNext()) {
      indent();
      iter.next()->node()->visit(this);
    }
  }

  bool needSemicolon(AstNode* node) {
    return node->isCallNode() || node->isBinaryOpNode()
        || node->isUnaryOpNode() || node->isStringLiteralNode()
        || node->isDoubleLiteralNode() || node->isIntLiteralNode()
        || node->isLoadNode();
  }

 public:
  ToCpp11Visitor(ostream & output = std::cout,
                 const uint8_t indentSpaces = 2) :
      _output(output), _indentSpaces(indentSpaces), _indentLevel(0) {
  }

  void insideBlock(BlockNode* node) {
    Scope* oldscope = _scope;
    _scope = node->scope();

    variableDeclarations(node->scope());
    functionDeclarations(node->scope());
    visitFunctions(node->scope());

    for (uint32_t i = 0; i < node->nodes(); i++) {
      indent();
      AstNode* current = node->nodeAt(i);
      current->visit(this);
      if (needSemicolon(current))
        _output << ";";
      _output << endl;
    }

    _scope = oldscope;
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
    _output << '"' << util::escape(node->literal()) << '"';
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
    _output << node->var()->name()
            << " = ";
    node->inExpr()->asBinaryOpNode()->left()->visit(this);
    _output << "; "; indent();
    _output << "for (; "
            << node->var()->name()
            << " <= ";
    node->inExpr()->asBinaryOpNode()->right()->visit(this);
    _output << "; ++" << node->var()->name() << ")";
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
      assert(false);
  }

  /**
   *  int foo(int x) {...}
   *     ==>
   *  std::function<int64_t(int64_t)> function_foo;
   *  function_foo = [&](int64_t x) -> int64_t {...};
   */
  virtual void visitFunctionNode(FunctionNode* node) {
    FunctionNode* old = _function;
    _function = node;
    if (node->name() != AstFunction::top_name) {
      if (!isNativeNode(node)) {
        _output << "function_"
                << node->name() << " = [&]"
                << "(";
        for (uint32_t i = 0; i < node->parametersNumber(); i++) {
          if (i != 0)
            _output << ", ";
          _output << typeToCppType(node->parameterType(i))
                  << " " << node->parameterName(i);
        }
        _output << ") -> " << typeToCppType(node->returnType());

        node->body()->visit(this);
        _output << ";" << endl;
      }
    } else {
      // main
      _output <<
          "#include <cstdint>\n"
          "#include <cassert>\n"
          "#include <dlfcn.h>\n"
          "#include <functional>\n"
          "#include <iostream>\n\n\n"
          "int main(void) {\n";
      blockEnter();
      NativeVisitor native_visitor(_output, _indentSpaces);
      node->body()->visit(&native_visitor);
      insideBlock(node->body());
      indent();
      blockExit();
      _output << "return 0;\n}\n";
    }
    _function = old;
  }

  virtual void visitReturnNode(ReturnNode* node) {
    _output << "return";
    if (_function->name() == AstFunction::top_name)
        _output << " 0";
    else if (node->returnExpr() != NULL) {
      _output << " ";
      node->returnExpr()->visit(this);
    }
    _output << ";";
  }

  virtual void visitCallNode(CallNode* node) {
    AstFunction* f = _scope->lookupFunction(node->name());
    assert(f);
    if (isNativeNode(f->node()))
      _output << "(*native_"
              << f->node()->body()->nodeAt(0)->asNativeCallNode()->nativeName()
              << ")";
    else
      _output << "function_" << node->name();

    // params
    _output << "(";
    for (uint32_t i = 0; i < node->parametersNumber(); i++) {
      if (i != 0)
        _output << ", ";
      node->parameterAt(i)->visit(this);
    }
    _output << ")";
  }

  virtual void visitPrintNode(PrintNode* node) {
    _output << "std::cout << ";
    for (uint32_t i = 0; i < node->operands(); i++) {
      if (i != 0)
        _output << " << ";
      node->operandAt(i)->visit(this);
    }
    _output << ";";
  }

};

class Magic : public Translator {
 public:
  virtual Status* translate(const string& program, Code* *code)  {
    Parser parser;
    Status* status = parser.parseProgram(program);
    if (status->isError()) return status;

    CppCode* cppCode = new CppCode;
    *code = cppCode;

    cppCode->cppCodeFilename = std::string("/tmp/cpp11_result.cpp");
    std::ofstream f(cppCode->cppCodeFilename.c_str());

    ToCpp11Visitor visitor(f);
    visitor.visitFunctionNode(parser.top()->node());

    return Status::Ok();
  }
};

Translator* Translator::create(const string& impl) {
  if (impl == "toCpp11") {
    return new Magic();
  } else {
    return NULL;
  }
}

}
