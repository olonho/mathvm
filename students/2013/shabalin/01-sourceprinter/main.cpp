#include <cstdlib>
#include <iostream>
#include <fstream>

#include "parser.h"

using namespace mathvm;

class PrinterVisitor : public AstVisitor {
   ostream& str;
   uint32_t indent_level;
   int last_precedence;
public:
   PrinterVisitor(ostream& ost): str(ost), indent_level(0) { }

   void run(AstFunction* top) {
      indent_level = -1;
      top->node()->body()->visit(this);
   }

   virtual void visitBinaryOpNode(BinaryOpNode* node) {
      bool parens_required = tokenPrecedence(node->kind()) < last_precedence;
      if(parens_required) str << "(";
      last_precedence = tokenPrecedence(node->kind());
      node->left()->visit(this);
      str << " " << tokenOp(node->kind()) << " ";
      node->right()->visit(this);
      if(parens_required) str << ")";
   }

   virtual void visitUnaryOpNode(UnaryOpNode* node) {
      str << tokenOp(node->kind());
      last_precedence = tokenPrecedence(node->kind());
      node->operand()->visit(this);
   }

   virtual void visitStringLiteralNode(StringLiteralNode* node) { 
      std::string lit = "";
      for(uint32_t i = 0; i < node->literal().size(); ++i) {
         char c = node->literal()[i];
         switch(c) {
            case '\n': lit.append("\\n"); break;
            case '\t': lit.append("\\t"); break;
            case '\r': lit.append("\\r"); break;
            case '\\': lit.append("\\\\"); break;
            case '\'': lit.append("\\'"); break;
            default: lit.push_back(c);
         }
      }
      str << "'" << lit << "'";
   }

   virtual void visitDoubleLiteralNode(DoubleLiteralNode* node) {
      str << node->literal();
   }

   virtual void visitIntLiteralNode(IntLiteralNode* node) {
      str << node->literal();
   }

   virtual void visitLoadNode(LoadNode* node) {
      str << node->var()->name();
   }

   virtual void visitStoreNode(StoreNode* node) {
      str << node->var()->name() << " " << tokenOp(node->op()) << " ";
      last_precedence = 0;
      node->value()->visit(this);
   }

   virtual void visitForNode(ForNode* node) {
      str << "for (" << node->var()->name() << " in ";
      last_precedence = 0;
      node->inExpr()->visit(this);
      str << ") {" << std::endl;
      node->body()->visit(this);
      printIndent();
      str << "}";
   }

   virtual void visitWhileNode(WhileNode* node) {
      str << "while (";
      last_precedence = 0;
      node->whileExpr()->visit(this);
      str << ") {" << std::endl;
      node->loopBlock()->visit(this);
      printIndent();
      str << "}";
   }

   virtual void visitIfNode(IfNode* node) {
      str << "if (";
      last_precedence = 0;
      node->ifExpr()->visit(this);
      str << ") {" << std::endl;
      node->thenBlock()->visit(this);
      if(node->elseBlock()) {
         printIndent();
         str << "} else {" << std::endl;;
         node->elseBlock()->visit(this);
      }
      printIndent();
      str << "}";
   }

   virtual void visitBlockNode(BlockNode* node) {
      ++indent_level;
      Scope::VarIterator v(node->scope());
      while(v.hasNext()) {
         printIndent();
         printVariableDeclaration(v.next());
      }
      if(node->scope()->variablesCount() > 0)
         str << std::endl;
      Scope::FunctionIterator f(node->scope());
      while(f.hasNext()) {
         printIndent();
         printFunctionDeclaration(f.next());
         str << std::endl;
      }
      for(uint32_t i = 0; i < node->nodes(); ++i) {
         printIndent();
         node->nodeAt(i)->visit(this);
         if(!node->nodeAt(i)->isForNode() &&
               !node->nodeAt(i)->isWhileNode() &&
               !node->nodeAt(i)->isIfNode())
            str << ";";
         str << std::endl;
      }
      --indent_level;
   }

   virtual void visitFunctionNode(FunctionNode* node) {
      str << "function " << typeToName(node->returnType()) << " " << node->name() << "(";
      if(node->parametersNumber() != 0) {
         str << typeToName(node->parameterType(0)) << " " << node->parameterName(0);
         for(uint32_t i = 1; i < node->parametersNumber(); ++i) {
            str << ", " << typeToName(node->parameterType(i)) << " " 
                        << node->parameterName(i);
         }
      }
      str << ")";
   }

   virtual void visitReturnNode(ReturnNode* node) {
      str << "return";
      if(node->returnExpr()) {
         str << " ";
         node->returnExpr()->visit(this);
      }
   }

   virtual void visitCallNode(CallNode* node) {
      str << node->name() << "(";
      if(node->parametersNumber() != 0) {
         last_precedence = 0;
         node->parameterAt(0)->visit(this);
         for(uint32_t i = 1; i < node->parametersNumber(); ++i) {
            str << ", ";
            last_precedence = 0;
            node->parameterAt(i)->visit(this);
         }
      }
      str << ")";
   }

   virtual void visitNativeCallNode(NativeCallNode* node) {
      str << "native '" << node->nativeName() << "'";
   }

   virtual void visitPrintNode(PrintNode* node) {
      str << "print(";
      if(node->operands() != 0) {
         last_precedence = 0;
         node->operandAt(0)->visit(this);
         for(uint32_t i = 1; i < node->operands(); ++i) {
            str << ", ";
            last_precedence = 0;
            node->operandAt(i)->visit(this);
         }
      }
      str << ")";
   }
private:
   void printIndent() {
      for(uint32_t j = 0; j < indent_level; ++j) str << "   ";
   }

   void printVariableDeclaration(AstVar* var) {
      str << typeToName(var->type()) << " " << var->name() << ";" << std::endl;
   }

   void printFunctionDeclaration(AstFunction* func) {
      func->node()->visit(this);
      if(func->node()->body()->nodeAt(0)->isNativeCallNode()) {
         str << " ";
         func->node()->body()->nodeAt(0)->visit(this);
         str << ";" << std::endl;
      } else {
         str << " {" << std::endl;
         func->node()->body()->visit(this);
         printIndent();
         str << "}" << std::endl;
      }
   }

};

void usage(char const* prog) {
   std::cerr << "USAGE: " << prog << " input-file" << std::endl;
   std::cerr << " input-file - source code for mvm language" << std::endl;
   std::cerr << " Program will print it back on stdout" << std::endl;
}

void get_source_code(char const* filename, std::string& source_code) {
   std::string buf;
   std::ifstream input(filename);
   while(std::getline(input, buf)) {
      source_code.append(buf);
      source_code.push_back('\n');
   }
}

int main(int argc, char** argv) {
   if(argc != 2) {
      usage(argv[0]);
      exit(1);
   }
   std::string source_code;
   get_source_code(argv[1], source_code);
   //std::cout << "--- ORIGINAL ---" << std::endl;
   //std::cout << source_code << std::endl;
   Parser parser;
   if(Status* s = parser.parseProgram(source_code)) {
      std::cerr << "ERROR: " << s->getError() << std::endl;
      exit(1);
   }
   AstFunction* top = parser.top();
   PrinterVisitor visitor(std::cout);
   //std::cout << "--- RECOVERED ---" << std::endl;
   visitor.run(top);
   return 0;
}
