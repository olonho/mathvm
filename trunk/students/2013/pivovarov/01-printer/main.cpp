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
   int last_precedence;

   #define SET_PRECEDENCE(KIND)                    \
      last_precedence = tokenPrecedence(KIND);

   #define WRAP_BRACKETS(DO)                       \
      os << "{";                                   \
      os << endl;                                  \
      SET_PRECEDENCE(tEOF);                        \
      DO                                           \
      SET_PRECEDENCE(tUNDEF);                      \
      printIndent();                               \
      os << "}";                                   \
      os << endl;

   #define VISIT(NODE)                             \
      NODE->visit(this);

   #define VISIT_STATEMENT(DO)                     \
      SET_PRECEDENCE(tEOF);                        \
      VISIT( DO );                                 \
      SET_PRECEDENCE(tUNDEF);

   #define WRAP_PRECEDENCE(KIND, DO)               \
      bool __parens =                              \
         tokenPrecedence(KIND) < last_precedence;  \
      if(__parens) os << "(";                      \
      DO                                           \
      if(__parens) os << ")";

   void printIndent() {
      for (uint i = 1; i < indent; ++i) {
         os << "  ";
      }
   }

   TokenKind VarType2TokenKind(VarType type) {
      switch(type) {
         case VT_DOUBLE:  return tDOUBLE; break;
         case VT_INT:     return tINT; break;
         case VT_STRING:  return tSTRING; break;
         case VT_VOID:
         case VT_INVALID:
         default:
            return tUNDEF;
      }
   }

public:
   PrinterVisitor(ostream & os) : os(os), indent(0) {}

   void run(AstFunction* top) {
      VISIT_STATEMENT( top->node()->body() );
   }

   virtual void visitBinaryOpNode(BinaryOpNode* node) {
      WRAP_PRECEDENCE ( node->kind(),
         SET_PRECEDENCE(node->kind());
         VISIT( node->left() );
         os << tokenOp(node->kind());
         SET_PRECEDENCE(node->kind());
         VISIT( node->right() );
         SET_PRECEDENCE(tUNDEF);
      )
   }

   virtual void visitUnaryOpNode(UnaryOpNode* node) {
      WRAP_PRECEDENCE ( node->kind(),
         os << tokenOp(node->kind());
         SET_PRECEDENCE(node->kind());
         VISIT( node->operand() );
         SET_PRECEDENCE(tUNDEF);
      )
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
      SET_PRECEDENCE(tSTRING);
   }

   virtual void visitDoubleLiteralNode(DoubleLiteralNode* node) {
      os << node->literal();
      SET_PRECEDENCE(tDOUBLE);
   }

   virtual void visitIntLiteralNode(IntLiteralNode* node) {
      os << node->literal();
      SET_PRECEDENCE(tINT);
   }

   virtual void visitLoadNode(LoadNode* node) {
      os << node->var()->name();
      SET_PRECEDENCE(VarType2TokenKind(node->var()->type()));
   }

   virtual void visitStoreNode(StoreNode* node) {
      os << node->var()->name();
      os << " ";
      os << tokenOp(node->op());
      os << " ";
      VISIT_STATEMENT( node->value() );
   }

   virtual void visitForNode(ForNode* node) {
      os << "for (";
      os << node->var()->name();
      os << " in ";
      VISIT_STATEMENT( node->inExpr() );
      os << ") ";

      WRAP_BRACKETS(
         VISIT( node->body() );
      )
   }

   virtual void visitWhileNode(WhileNode* node) {
      os << "while (";
      VISIT_STATEMENT( node->whileExpr() );
      os << ") ";
      WRAP_BRACKETS(
         VISIT( node->loopBlock() );
      )
   }

   virtual void visitIfNode(IfNode* node) {
      os << "if (";
      VISIT_STATEMENT( node->ifExpr() );
      os << ") ";
      WRAP_BRACKETS(
         VISIT( node->thenBlock() );
      )
      if (node->elseBlock()) {
         os << " else ";
         WRAP_BRACKETS(
            VISIT( node->elseBlock() );
         )
      }
   }

   virtual void visitBlockNode(BlockNode* node) {
      indent++;

      Scope* scope = node->scope();

      Scope::VarIterator v(scope);
      while(v.hasNext()) {
         printIndent();
         AstVar* var = v.next();
         os << typeToName(var->type());
         os << " ";
         os << var->name();
         os << ";";
         os << endl;
      }
      if (scope->variablesCount() > 0) {
         os << endl;
      }

      Scope::FunctionIterator f(scope);
      while(f.hasNext()) {
         printIndent();
         AstFunction* fun = f.next();
         VISIT( fun->node() );
         os << endl;
      }
      if (scope->functionsCount() > 0) {
         os << endl;
      }

      for (uint i = 0; i < node->nodes(); ++i) {
         printIndent();
         VISIT_STATEMENT( node->nodeAt(i) );

         if(!node->nodeAt(i)->isForNode()   &&
            !node->nodeAt(i)->isWhileNode() &&
            !node->nodeAt(i)->isIfNode()) {
               os << ";";
               os << endl;
         }
      }

      indent--;
   }

   virtual void visitFunctionNode(FunctionNode* node) {
      os << "function ";
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
      os << ") ";
      bool isNative = node->body()->nodes() > 0 && node->body()->nodeAt(0)->isNativeCallNode();
      if (isNative) {
         VISIT( node->body()->nodeAt(0) );
      } else {
         WRAP_BRACKETS(
            VISIT( node->body() );
         )
      }
   }

   virtual void visitReturnNode(ReturnNode* node) {
      os << "return";
      if (node->returnExpr()) {
         os << " ";
         VISIT_STATEMENT( node->returnExpr() );
      }
   }

   virtual void visitCallNode(CallNode* node) {
      os << node->name();
      os << "(";
      if (node->parametersNumber() > 0) {
         VISIT_STATEMENT( node->parameterAt(0) );
         for (uint i = 1; i < node->parametersNumber(); ++i) {
            os << ", ";
            VISIT_STATEMENT( node->parameterAt(i) );
         }
      }
      os << ")";
   }

   virtual void visitNativeCallNode(NativeCallNode* node) {
      os << "native '";
      os << node->nativeName();
      os << "'";
   }

   virtual void visitPrintNode(PrintNode* node) {
      os << "print (";
         if (node->operands() > 0) {
         VISIT_STATEMENT( node->operandAt(0) );
         for (uint i = 1; i < node->operands(); ++i) {
            os << ", ";
            VISIT_STATEMENT( node->operandAt(i) );
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
