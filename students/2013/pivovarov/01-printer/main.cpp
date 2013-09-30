#include <cstdlib>
#include <iostream>
#include <fstream>

#include "parser.h"

using namespace mathvm;

using std::string;
using std::cout;
using std::cerr;
using std::endl;
using std::ifstream;
using std::ostream;
using std::pair;

class PrinterVisitor : public AstVisitor {
private:
   ostream & os;
   uint indent;

   #define WRAP_BRACKETS(DO)  \
      os << "{";              \
      os << endl;             \
      DO                      \
      printIndent();          \
      os << "}";              \
      os << endl;

   #define VISIT(NODE)        \
      NODE->visit(this);

public:
   PrinterVisitor(ostream & os) : os(os), indent(0) {}

   void run(AstFunction* top) {
      VISIT( top->node()->body() );
   }

   void printIndent() {
      for (uint i = 1; i < indent; ++i) {
         os << "  ";
      }
   }

   virtual void visitBinaryOpNode(BinaryOpNode* node) {
      os << "(";
      VISIT( node->left() );
      os << ")";
      os << tokenOp(node->kind());
      os << "(";
      VISIT( node->right() );
      os << ")";
   }

   virtual void visitUnaryOpNode(UnaryOpNode* node) {
      os << tokenOp(node->kind());
      os << "(";
      VISIT( node->operand() );
      os << ")";
   }

   virtual void visitStringLiteralNode(StringLiteralNode* node) {
      os << "'";
      string const & s = node->literal();
      for (uint i = 0; i < s.size(); ++i) {
         switch(s[i]) {
            case '\n': os << "\\n";  break;
            case '\t': os << "\\t";  break;
            case '\r': os << "\\r";  break;
            case '\\': os << "\\\\"; break;
            case '\'': os << "\\'";  break;
            default:
               os << s[i];
         }
      }
      os << "'";
   }

   virtual void visitDoubleLiteralNode(DoubleLiteralNode* node) {
      os << node->literal();
   }

   virtual void visitIntLiteralNode(IntLiteralNode* node) {
      os << node->literal();
   }

   virtual void visitLoadNode(LoadNode* node) {
      os << node->var()->name();
   }

   virtual void visitStoreNode(StoreNode* node) {
      os << node->var()->name();
      os << " ";
      os << tokenOp(node->op());
      os << " ";
      VISIT( node->value() );
   }

   virtual void visitForNode(ForNode* node) {
      os << "for";
      os << " ";
      os << "(";
      os << node->var()->name();
      os << " ";
      os << "in";
      os << " ";
      VISIT( node->inExpr() );
      os << ")";
      os << " ";

      WRAP_BRACKETS(
         VISIT( node->body() );
      )
   }

   virtual void visitWhileNode(WhileNode* node) {
      os << "while";
      os << " ";
      os << "(";
      VISIT( node->whileExpr() );
      os << ")";
      os << " ";
      WRAP_BRACKETS(
         VISIT( node->loopBlock() );
      )
   }

   virtual void visitIfNode(IfNode* node) {
      os << "if";
      os << " ";
      os << "(";
      VISIT( node->ifExpr() );
      os << ")";
      os << " ";
      WRAP_BRACKETS(
         VISIT( node->thenBlock() );
      )
      if (node->elseBlock()) {
         os << " ";
         os << "else";
         os << " ";
         WRAP_BRACKETS(
            VISIT( node->elseBlock() );
         )
      }
   }

   virtual void visitBlockNode(BlockNode* node) {
      indent++;

      Scope::VarIterator v(node->scope());
      while(v.hasNext()) {
         printIndent();
         AstVar* var = v.next();
         os << typeToName(var->type());
         os << " ";
         os << var->name();
         os << ";";
         os << endl;
      }
      Scope::FunctionIterator f(node->scope());
      while(f.hasNext()) {
         printIndent();
         AstFunction* fun = f.next();
         VISIT( fun->node() );
         os << endl;
      }

      for (uint i = 0; i < node->nodes(); ++i) {
         printIndent();
         VISIT( node->nodeAt(i) );

         if(!node->nodeAt(i)->isForNode() &&
            !node->nodeAt(i)->isWhileNode() &&
            !node->nodeAt(i)->isIfNode()) {
               os << ";";
               os << endl;
         }
      }

      indent--;
   }

   virtual void visitFunctionNode(FunctionNode* node) {
      os << "function";
      os << " ";
      os << typeToName(node->returnType());
      os << " ";
      os << node->name();
      os << "(";
      if (node->parametersNumber() > 0) {
         os << typeToName(node->parameterType(0));
         os << " ";
         os << node->parameterName(0);
         for (uint i = 1; i < node->parametersNumber(); ++i) {
            os << ", ";
            os << typeToName(node->parameterType(i));
            os << " ";
            os << node->parameterName(i);
         }
      }
      os << ")";
      os << " ";
      WRAP_BRACKETS(
         VISIT( node->body() );
      )
   }

   virtual void visitReturnNode(ReturnNode* node) {
      os << "return";
      if (node->returnExpr()) {
         os << " ";
         VISIT( node->returnExpr() );
      }
      os << ";";
   }

   virtual void visitCallNode(CallNode* node) {
      os << node->name() << "(";
      if (node->parametersNumber() > 0) {
         VISIT( node->parameterAt(0) );
         for (uint i = 1; i < node->parametersNumber(); ++i) {
            os << ", ";
            VISIT( node->parameterAt(i) );
         }
      }
      os << ")";
   }

   virtual void visitNativeCallNode(NativeCallNode* node) {
      os << "native";
      os << "'";
      os << node->nativeName();
      os << "'";
   }

   virtual void visitPrintNode(PrintNode* node) {
      os << "print";
      os << "(";
         if (node->operands() > 0) {
         VISIT( node->operandAt(0) );
         for (uint i = 1; i < node->operands(); ++i) {
            os << ", ";
            VISIT( node->operandAt(i) );
         }
      }
      os << ")";
   }
};

void get_source_code(char const * filename, string & source_code) {
   string buf;
   ifstream input(filename);
   while(std::getline(input, buf)) {
      source_code.append(buf);
      source_code.push_back('\n');
   }
}

int main(int argc, char** argv) {
   if(argc != 2) {
      cout << "Usage: " << argv[0] << " <input>" << endl;
      return 1;
   }

   string source_code;
   get_source_code(argv[1], source_code);
   Parser parser;
   if(Status* s = parser.parseProgram(source_code)) {
      cerr << "ERROR: " << s->getError() << endl;
      exit(1);
   }

   AstFunction* top = parser.top();
   PrinterVisitor visitor(cout);
   visitor.run(top);

   return 0;
}
