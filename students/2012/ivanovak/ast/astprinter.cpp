#include <iostream>
#include <string>

#include "astprinter.h"
#include "ast.h"

ASTPrinter::ASTPrinter() {}
ASTPrinter::~ASTPrinter() {}

inline void printEscaped(char ch) {
   switch (ch) {
      case '\\': std::cout << "\\\\"; break;
      case '\n': std::cout << "\\n"; break;
      case '\t': std::cout << "\\t"; break;
      case '\r': std::cout << "\r"; break;
      default: std::cout << ch;
   }
}

inline void printVar(const mathvm::AstVar *var) {
    std::cout << mathvm::typeToName(var->type()) << " " << var->name();
}

void ASTPrinter::visitBlockNode(mathvm::BlockNode *node) {
    mathvm::Scope::VarIterator var(node->scope());

    while(var.hasNext()) {
	   printVar(var.next());
	   std::cout << ";" << std::endl;
    }
    
    mathvm::Scope::FunctionIterator func(node->scope());
    while(func.hasNext()) func.next()->node()->visit(this);

    for (uint32_t i = 0; i < node->nodes(); ++i) {
    	node->nodeAt(i)->visit(this);
	   std::cout << ";" << std::endl;
    }
}

void ASTPrinter::visitForNode(mathvm::ForNode *node) {
    std::cout << "for (";
    printVar(node->var());
    std::cout << " in ";
    node->inExpr()->visit(this);
    std::cout << ") {" << std::endl;
    node->body()->visit(this);
    std::cout << "}" << std::endl;
}

void ASTPrinter::visitPrintNode(mathvm::PrintNode *node) {
    std::cout << "print(";
    for (uint32_t i = 0; i < node->operands(); i++) {
        if (i > 0) std::cout << ", ";
        mathvm::AstNode *pNode = node->operandAt(i);
        pNode->visit(this);
    }
    std::cout << ")";
}

void ASTPrinter::visitLoadNode(mathvm::LoadNode *node) {
    std::cout << node->var()->name();
}

void ASTPrinter::visitIfNode(mathvm::IfNode *node) {
    std::cout << "if (";
    node->ifExpr()->visit(this);
    std::cout << ") {" << std::endl;
    node->thenBlock()->visit(this);
    std::cout << "}";
    if (node->elseBlock()) {
        std::cout << "else {" << std::endl;
        node->elseBlock()->visit(this);
        std::cout << "}";
    }
    std::cout << std::endl;
}

void ASTPrinter::visitCallNode(mathvm::CallNode *node) {
    std::cout << node->name() << "(";
    for (uint32_t i = 0; i < node->parametersNumber(); i++) {
        if (i > 0) std::cout << ", ";
        node->parameterAt(i)->visit(this);
    }
    std::cout << ")";
}

void ASTPrinter::visitDoubleLiteralNode(mathvm::DoubleLiteralNode *node) {
    std::cout << node->literal();
}

void ASTPrinter::visitStoreNode(mathvm::StoreNode *node) {
    std::cout << node->var()->name() << 
      " " << 
      std::string(tokenOp(node->op())) << 
      " "; 
    node->value()->visit(this);
}

void ASTPrinter::visitStringLiteralNode(mathvm::StringLiteralNode *node) {
    std::cout << "'";
    
    for (uint32_t i = 0; i < node->literal().length(); ++i)
	   printEscaped(node->literal()[i]); 
	   
    std::cout << "'";
}

void ASTPrinter::visitWhileNode(mathvm::WhileNode *node) {
    std::cout << "while (";
    node->whileExpr()->visit(this);
    std::cout << ") {" << std::endl;
    node->loopBlock()->visit(this);
    std::cout << "}" << std::endl;
}

void ASTPrinter::visitIntLiteralNode(mathvm::IntLiteralNode *node) {
    std::cout << node->literal();
}

void ASTPrinter::visitFunctionNode(mathvm::FunctionNode *node) {
    if(node->name() == "<top>") {
	   node->body()->visit(this);
	   return;
    }
    
    std::cout << "function " << typeToName(node->returnType()) << " " << node->name() << "(";
    
    for (uint32_t j = 0; j < node->parametersNumber(); j++) {
        if (j > 0) 
         std::cout << ", ";
        std::cout << typeToName(node->parameterType(j)) << " " << node->parameterName(j);
    }
    
    std::cout << ") ";
    if (node->body()->nodes() > 0 && node->body()->nodeAt(0)->isNativeCallNode()) {
   	node->body()->nodeAt(0)->visit(this);
    } else {
   	std::cout << "{" << std::endl;
    	node->body()->visit(this);
    	std::cout << "}" << std::endl;
    }
}

void ASTPrinter::visitBinaryOpNode(mathvm::BinaryOpNode *node) {
    node->left()->visit(this);
    std::cout << std::string(tokenOp(node->kind()));
    node->right()->visit(this);
}

void ASTPrinter::visitUnaryOpNode(mathvm::UnaryOpNode *node) {
    std::cout << std::string(tokenOp(node->kind()));
    node->operand()->visit(this);
}

void ASTPrinter::visitReturnNode(mathvm::ReturnNode *node) {
    std::cout << "return ";
    if(node->returnExpr() != 0) 
      node->returnExpr()->visit(this);
}

void ASTPrinter::visitNativeCallNode(mathvm::NativeCallNode *node) {
    std::cout << "native '"<< node->nativeName() << "';" << std::endl;
}
