#include <iostream>

#include "mathvm.h"
#include "parser.h"
#include "visitors.h"

namespace mathvm {

  /* AstDumper implementation */

  const char* tokRepr[] = {
#define TOK_REPR(t, s, p) s,
    FOR_TOKENS(TOK_REPR)
#undef TOK_REPR
  };

  static int indent; // Need to be encapsulated into the AstDumper class
  
  static void escape_string(const std::string& s) {
    for (std::string::const_iterator c = s.begin();
         c != s.end();
         ++c) {
      if (*c == '\"') {
        std::cout << "\\\"";
      } else if (*c == '\n') {
        std::cout << "\\n";
      } else {
        std::cout << *c;
      }
    }
  }

  static void print_indent() {
    for(int i = 0; i < indent*2; ++i) {
      std::cout << ' ';
    }
  }

#define VISIT_METHOD(type)                      \
  void AstDumper::visit##type(type* node)


  void AstDumper::dump(AstNode* root) {
    indent = 0;
    root->visit(this);
  }
    
  VISIT_METHOD(BinaryOpNode) {
    node->left()->visit(this);
    std::cout << ' ' << tokRepr[node->kind()] << ' ';
    node->right()->visit(this);
  }

  VISIT_METHOD(UnaryOpNode) {
    std::cout << tokRepr[node->kind()];
    node->operand()->visit(this);
  }

  VISIT_METHOD(StringLiteralNode) {
    std::cout << "\"";
    escape_string(node->literal());
    std::cout << "\"";
  }
     
  VISIT_METHOD(DoubleLiteralNode) {
    std::cout << node->literal();
  }

  VISIT_METHOD(IntLiteralNode) {
    std::cout << node->literal();
  }

  VISIT_METHOD(LoadNode) {
    std::cout << node->var()->name();
  }

  VISIT_METHOD(StoreNode) {
    std::cout << node->var()->name() << " = ";
    node->visitChildren(this);
    std::cout << "; ";
  }
    
  VISIT_METHOD(ForNode) {
    //print_indent();
    std::cout << "for (" << node->var()->name() << " in ";
    node->inExpr()->visit(this);
    std::cout << ") {\n";

    indent++;
    node->body()->visit(this);

    indent--;
    print_indent();
    std::cout << "}";
  }
  
  VISIT_METHOD(IfNode) {
    //print_indent();
    std::cout << "if (";
    node->ifExpr()->visit(this);
    std::cout << ") {\n";

    indent++;
    node->thenBlock()->visit(this);

    if (node->elseBlock()) {
      indent--;
      print_indent();
      std::cout << "} else {\n";
      
      indent++;
      node->elseBlock()->visit(this);
    }

    indent--;
    print_indent();
    std::cout << "}\n";
  }

  VISIT_METHOD(WhileNode) {
    //print_indent();
    std::cout << "while (";
    node->whileExpr()->visit(this);
    std::cout << ") {\n";

    indent++;
    node->loopBlock()->visit(this);

    indent--;
    print_indent();
    std::cout << "}\n";
  }

  VISIT_METHOD(BlockNode) {
    for (uint32_t i = 0; i < node->nodes(); i++) {
      print_indent();
      node->nodeAt(i)->visit(this);
      std::cout << "\n";
    }
  }

  VISIT_METHOD(FunctionNode) {
    node->visitChildren(this);
  }
    
  VISIT_METHOD(PrintNode) {
    uint32_t i;

    std::cout << "print(";

    for (i = 0; i < node->operands() - 1; i++) {
      node->operandAt(i)->visit(this);
      std::cout << ", ";
    }
    node->operandAt(i)->visit(this);

    std::cout << ");";
  }

  #undef VISIT_METHOD
  
  // --------------------------------------------------------------------------------

  class ParsingTranslator : public Translator {
    class ASTCode : public Code {
    public:
      ASTCode(Parser* p)
        : parser(p) { }

      Status* execute(vector<Var*> vars) { 
        AstDumper dumper;

        dumper.dump(parser->top());

        return 0;
      }

    private:
      Parser* parser;
    };

  public:
    Status* translate(const string& program, Code** code) {
      Status *status = parser.parseProgram(program);
      if (!status) {
        *code = new ASTCode(&parser);
      }

      return status;
    }

  private:
    Parser parser;
  };

  // --------------------------------------------------------------------------------

  // Implement me!
  Translator* Translator::create(const string& impl) {
    /*
      if (impl == "" || impl == "intepreter") {
      //return new BytecodeTranslatorImpl();
      return 0;
    }
    assert(false);
    return 0;
    */

    return new ParsingTranslator;
  }
  
}
