#include "mathvm.h"
#include "parser.h"
#include "visitors.h"

#include <iostream>
#include <map>
#include <string>
#include <utility>

namespace mathvm {
  class AstPrinter : public Translator {
    public:
      virtual Status* translate(const string& program, Code* *code);
  };

  class AstPrinterVisitor : public AstBaseVisitor {
    public:
      AstPrinterVisitor(std::ostream& out): out_(out), level_(0), tab_size_(4) {
        #define INSERT(t, s, p) tokens_.insert(std::make_pair(t, s));
          FOR_TOKENS(INSERT)
        #undef INSERT
      }
      
      virtual void visitBlockNode(BlockNode* node);
      virtual void visitFunctionNode(FunctionNode* node);
      virtual void visitNativeCallNode(NativeCallNode* node);
      virtual void visitForNode(ForNode* node);
      virtual void visitIfNode(IfNode* node);
      virtual void visitWhileNode(WhileNode* node);
      virtual void visitLoadNode(LoadNode* node);
      virtual void visitStoreNode(StoreNode* node);
      virtual void visitPrintNode(PrintNode* node);
      virtual void visitReturnNode(ReturnNode* node);
      virtual void visitCallNode(CallNode* node);
      virtual void visitBinaryOpNode(BinaryOpNode* node);
      virtual void visitUnaryOpNode(UnaryOpNode* node);
      virtual void visitDoubleLiteralNode(DoubleLiteralNode* node);
      virtual void visitIntLiteralNode(IntLiteralNode* node);
      virtual void visitStringLiteralNode(StringLiteralNode* node);

    private:
      void visitScope(Scope* scope);

      void indent() {
        for (uint32_t i = tab_size_; i < level_ * tab_size_; ++i) {
          out_ << ' ';
        }
      }

      std::ostream& out_;
      std::map<TokenKind, std::string> tokens_; 
      uint32_t level_;
      const uint32_t tab_size_;
  };
}

using namespace mathvm;

Translator* Translator::create(const string& impl) {
    if (impl == "printer") {
      return new AstPrinter();
    } 

    return NULL;
}


Status* AstPrinter::translate(const string& program, Code* *code) {
  Parser parser;
  
  Status* status = parser.parseProgram(program);
  if (status->isError()) return status;

  AstPrinterVisitor visitor(std::cout);
  parser.top()->node()->body()->visit(&visitor);

  return Status::Ok();
}


void AstPrinterVisitor::visitBlockNode(BlockNode* node) {
  bool on_top_level = ++level_ <= 1;

  if (!on_top_level) {
    out_ << "{" << std::endl;
  }

  visitScope(node->scope());

  for (uint32_t i = 0; i < node->nodes(); ++i) {
    AstNode* statement = node->nodeAt(i);
    
    indent();
    statement->visit(this);

    if (statement->isCallNode()) {
      out_ << ";";
    }    

    out_ << std::endl;
  }

  level_--;
  if (!on_top_level) {
    indent();
    out_ << "}";
  }

}

void AstPrinterVisitor::visitFunctionNode(FunctionNode* node) {
  out_ << "function " 
       << typeToName(node->returnType()) 
       << " "
       << node->name() 
       << "(";
  
  for (uint32_t i = 0; i < node->parametersNumber(); ++i) {
    if (i != 0) {
      out_ << ", "; 
    }

    out_ << typeToName(node->parameterType(i)) 
         << " "
         << node->parameterName(i);
  }

  out_ << ") ";

  if (node->body()->nodes() > 0 && node->body()->nodeAt(0)->isNativeCallNode()) {
    node->body()->nodeAt(0)->visit(this);
  } else {
    node->body()->visit(this);
  }
  out_ << std::endl;
}

void AstPrinterVisitor::visitNativeCallNode(NativeCallNode* node) {
  out_ << "native '" << node->nativeName() << "';"; 
}

void AstPrinterVisitor::visitForNode(ForNode* node) {
  out_ << "for ("
       << node->var()->name()
       << " in ";
  node->inExpr()->visit(this);  
  out_ << ") ";
  node->body()->visit(this);
}

void AstPrinterVisitor::visitIfNode(IfNode* node) {
  out_ << "if (";
  node->ifExpr()->visit(this);
  out_ << ") ";
  node->thenBlock()->visit(this);

  if (node->elseBlock()) {
    out_ << " else ";
    node->elseBlock()->visit(this);
  }
}

void AstPrinterVisitor::visitWhileNode(WhileNode* node) {
  out_ << "while (";
  node->whileExpr()->visit(this);
  out_ << ") ";
  node->loopBlock()->visit(this);
}

void AstPrinterVisitor::visitLoadNode(LoadNode* node) {
  out_ << node->var()->name();
}

void AstPrinterVisitor::visitStoreNode(StoreNode* node) {
  out_ << node->var()->name() 
       << " = ";
  node->value()->visit(this); 
  out_ << ";";
}

void AstPrinterVisitor::visitPrintNode(PrintNode* node) {
  out_ << "print(";
  
  for (uint32_t i = 0; i < node->operands(); ++i) {
    if (i > 0) {
      out_ << ", ";
    }

    node->operandAt(i)->visit(this);
  }

  out_ << ");";
}

void AstPrinterVisitor::visitReturnNode(ReturnNode* node) {
  out_ << "return";
  
  if (node->returnExpr()) {
    out_ << " ";
    node->returnExpr()->visit(this);
  }

  out_ << ";";
}

void AstPrinterVisitor::visitCallNode(CallNode* node) {
  out_ << node->name() << "(";

    for (uint32_t i = 0; i < node->parametersNumber(); ++i) {
      if (i > 0) {
        out_ << ", ";
      }

      node->parameterAt(i)->visit(this);
    }

  out_ << ")";
}

void AstPrinterVisitor::visitBinaryOpNode(BinaryOpNode* node) {
  out_ << "(";
  node->left()->visit(this);
  out_ << tokens_[node->kind()];
  node->right()->visit(this);
  out_ << ")";
}

void AstPrinterVisitor::visitUnaryOpNode(UnaryOpNode* node) {
  out_ << "(";
  out_ << tokens_[node->kind()];
  node->operand()->visit(this);
  out_ << ")";
}

void AstPrinterVisitor::visitStringLiteralNode(StringLiteralNode* node) {
  out_ << "'";

  const std::string& literal = node->literal();
  for (uint32_t i = 0; i < literal.size(); ++i) {
    switch (literal[i]) {
      case '\n':  out_ << "\\n";  break;
      case '\r':  out_ << "\\r";  break;
      case '\t':  out_ << "\\t";  break;
      case '\\':  out_ << "\\\\"; break;
      case '\'':  out_ << "\\'";  break;
      case '\"':  out_ << "\\\""; break;
      default:
        out_ << literal[i];
    }
  }

  out_ << "'";
}

void AstPrinterVisitor::visitDoubleLiteralNode(DoubleLiteralNode* node) {
  out_ << node->literal();
}

void AstPrinterVisitor::visitIntLiteralNode(IntLiteralNode* node) {
  out_ << node->literal();
}

void AstPrinterVisitor::visitScope(Scope* scope) {
  Scope::VarIterator var_it(scope);
  while (var_it.hasNext()) {
    AstVar* var = var_it.next();
    
    indent();
    out_ << typeToName(var->type()) << " " << var->name() << ";" << std::endl;  
  }

  Scope::FunctionIterator func_it(scope);
  while (func_it.hasNext()) {
    indent();
    func_it.next()->node()->visit(this); 
  }
}