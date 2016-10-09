#include "visitors.h"
#include "parser.h"
#include <map>

namespace mathvm {

class AstPrinterVisitor : public AstVisitor {
public:
    AstPrinterVisitor(ostream& out) :
        out_(out),
        indent_(1) { }

    virtual void visitBinaryOpNode(BinaryOpNode* node) {
      node->left()->visit(this);
      out_ << " " << tokenOp(node->kind()) << " ";
      node->right()->visit(this);
    }

    virtual void visitUnaryOpNode(UnaryOpNode* node) {
      out_ << tokenOp(node->kind());
      node->operand()->visit(this);
    }

    virtual void visitStringLiteralNode(StringLiteralNode* node) {
      out_ << "'" << escapeString(node->literal()) << "'";
    }

    virtual void visitDoubleLiteralNode(DoubleLiteralNode* node) {
      out_ << node->literal();
    }

    virtual void visitIntLiteralNode(IntLiteralNode* node) {
      out_ << node->literal();
    }

    virtual void visitLoadNode(LoadNode* node) {
      out_ << node->var()->name();
    }

    virtual void visitStoreNode(StoreNode* node) {
      out_ << node->var()->name() << " = ";
      node->value()->visit(this);
      out_ << ";";
    }

    virtual void visitForNode(ForNode* node) {
      out_ << "for (" << node->var()->name() << " in ";
      node->inExpr()->visit(this);
      out_ << ")";
      node->body()->visit(this);
    }

    virtual void visitWhileNode(WhileNode* node) {
      out_ << "while ";
      node->whileExpr()->visit(this);
      node->loopBlock()->visit(this);
    }

    virtual void visitIfNode(IfNode* node) {
      out_ << "if (";
      node->ifExpr()->visit(this);
      out_ << ")";
      node->thenBlock()->visit(this);

      if (node->elseBlock() != NULL) {
        out_ << " else ";
        node->elseBlock()->visit(this);
      }
    }

    virtual void visitBlockNode(BlockNode* node) {
      bool top_level = indent_ == 0;
      ++indent_;
      if (!top_level) {
        out_ << " {" << endl;
        printScopeVars(node->scope());
      }

      for (size_t i = 0; i < node->nodes(); ++i) {
        append_tabs();
        node->nodeAt(i)->visit(this);
        if (node->nodeAt(i)->isCallNode()) {
          out_ << ";";
        }
        out_ << endl;
      }

      --indent_;
      append_tabs();
      if (!top_level) {
        out_ << "}" << endl;
      }
    }

    virtual void visitFunctionNode(FunctionNode* node) {
      if (node->name() != AstFunction::top_name) {
        out_ << "function " << typeName(node->returnType()) << " " << node->name() << "(";
        for (size_t i = 0; i < node->parametersNumber(); ++i) {
          out_ << typeName(node->parameterType(i)) << " " << node->parameterName(i);
          if (i + 1 < node->parametersNumber()) {
            out_ << ", ";
          }
        }
        out_ << ")";
      } else {
        indent_ = 0;
      }

      if (node->body()->nodes() > 0 && node->body()->nodeAt(0)->isNativeCallNode()) {
        node->body()->nodeAt(0)->visit(this);
      } else {
        node->body()->visit(this);
      }
    }

    virtual void visitReturnNode(ReturnNode* node) {
      out_ << "return";
      if (node->returnExpr() != NULL) {
        out_ << " ";
        node->returnExpr()->visit(this);
      }
      out_ << ";";
    }

    virtual void visitCallNode(CallNode* node) {
      out_ << node->name() << "(";
      for (size_t i = 0; i < node->parametersNumber(); ++i) {
        node->parameterAt(i)->visit(this);
        if (i + 1 < node->parametersNumber()) {
          out_ << ", ";
        }
      }
      out_ << ")";
    }

    virtual void visitPrintNode(PrintNode* node) {
      out_ << "print(";
      for (size_t i = 0; i < node->operands(); ++i) {
        node->operandAt(i)->visit(this);
        if (i + 1 < node->operands()) {
          out_ << ", ";
        }
      }
      out_ << ");";
    }

    virtual void visitNativeCallNode(NativeCallNode* node) {
      out_ << " native '" << node->nativeName() << "';" << endl;
    }

    void printScopeVars(Scope* scope) {
      Scope::VarIterator var_iter(scope);

      while (var_iter.hasNext()) {
        AstVar* var = var_iter.next();
        append_tabs();
        out_ << typeName(var->type()) << " " << var->name() << ";" << endl;
      }
    }

private:
    void append_tabs() {
      for (size_t i = 1; i < indent_; ++i) {
        out_ << "    ";
      }
    }

    string typeName(VarType type) {
      switch (type) {
        case VT_VOID:
          return "void";
        case VT_INT:
          return "int";
        case VT_DOUBLE:
          return "double";
        case VT_STRING:
          return "string";
        case VT_INVALID:
        default:
          return "invalid";
      }
    }

    string escaped(char ch) {
      switch (ch) {
        case '\n':
          return "\\n";
        case '\t':
          return "\\t";
        case '\r':
          return "\\r";
        case '\\':
          return "\\\\";
        case '\'':
          return "\\'";
        default:
          return string(1, ch);
      }
    }

    string escapeString(const string& s) {
      string result;
      for (size_t i = 0; i < s.length(); ++i) {
        result += escaped(s[i]);
      }
      return result;
    }

    ostream& out_;
    size_t indent_;
};

class AstPrinter : public Translator {
    virtual Status* translate(string const& src, Code** code) {
      Parser parser;
      Status* status = parser.parseProgram(src);
      if (status != NULL && status->isError()) {
        return status;
      }

      Scope::FunctionIterator it(parser.top()->scope()->childScopeAt(0));
      AstPrinterVisitor visitor(cout);

      visitor.printScopeVars(parser.top()->scope()->childScopeAt(0));

      while (it.hasNext()) {
        it.next()->node()->visit(&visitor);
      }

      AstNode* root = parser.top()->node();
      root->visit(&visitor);
      return Status::Ok();
    }
};

Translator* Translator::create(const string& impl) {
  return new AstPrinter();
}
}
