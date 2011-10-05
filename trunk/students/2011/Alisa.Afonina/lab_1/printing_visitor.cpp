#include <iostream>
#include <sstream>
#include <stdio.h>

#include "mathvm.h"
#include "ast.h"
#include "printing_visitor.h"


using namespace std;
using namespace mathvm;
void PrintingVisitor :: visitBinaryOpNode(BinaryOpNode* node) {
	cout << "( ";
	node->left()->visit(this);
	cout << tokenOp(node->kind());
	node->right()->visit(this);
	cout << ")";
}

void PrintingVisitor ::  visitUnaryOpNode(UnaryOpNode* node) {
	cout << tokenOp(node->kind());
	node->operand()->visit(this);
}

void PrintingVisitor ::  visitStringLiteralNode(StringLiteralNode* node) {
	cout << "\'";
	const string& literal = node->literal();
    for (unsigned int i = 0; i < literal.length(); i++ ) {
		char c = literal[i];
        switch (c) {
            case '\a': cout << "\\a"; break;
            case '\b': cout << "\\b"; break;
            case '\t': cout << "\\t"; break;
            case '\n': cout << "\\n"; break;
            case '\f': cout << "\\f"; break;
            case '\r': cout << "\\r"; break;
            case '\'': cout << "\\'"; break;
            case '\\': cout << "\\\\"; break;
            case 0   : cout << c;
        }
    }
    cout << "\'";
}

void PrintingVisitor ::  visitDoubleLiteralNode(DoubleLiteralNode* node) {
	  stringstream stream;
	  stream << node->literal();
	  string s = stream.str();
	  int pos = s.find("e+");
	  if (pos != -1) {
		s.replace(pos, 2, "e");
	  }
	  cout << s;
}

void PrintingVisitor ::  visitIntLiteralNode(IntLiteralNode* node) {
	cout << node->literal();
}

void PrintingVisitor ::  visitLoadNode(LoadNode* node) {
	cout << node->var()->name();
}

void PrintingVisitor ::  visitStoreNode(StoreNode* node) {
	cout << node->var()->name() << " " << tokenOp(node->op())<< " ";
	node->value()->visit(this);
	cout << ";" << endl;
}

void PrintingVisitor ::  visitForNode(ForNode* node) {
	cout << " for(" << node->var()->name() 
		 << " in " << node->inExpr() << ") {" << endl;
	node->body()->visit(this);
	cout << "}" << endl;	
}

void PrintingVisitor ::  visitWhileNode(WhileNode* node) {
	cout << "while (";
	node->whileExpr()->visit(this);
	cout << ") {" << endl;
	node->loopBlock()->visit(this);
	cout << "}" << endl;	
}

void PrintingVisitor ::  visitIfNode(IfNode* node) {
	cout << "if (";
	node->ifExpr()->visit(this);
	cout << ") {" << endl;
	node->thenBlock()->visit(this);
	if(node->elseBlock()) {
		cout << "} else {" << endl;
		node->elseBlock()->visit(this);	
	}
	cout << "}" << endl;	

}

void PrintingVisitor ::  visitBlockNode(BlockNode* node) {
	Scope::VarIterator it(node->scope());
	while (it.hasNext()) {
		AstVar* var = it.next();
		cout << getType(var->type()) << " " << var->name() << ";"<< endl;
	}
	node->visitChildren(this);
}


void PrintingVisitor :: visitFunctionNode(FunctionNode* node) {
}
void PrintingVisitor :: visitReturnNode(ReturnNode* node) {
}
void PrintingVisitor :: visitCallNode(CallNode* node) {
}

void PrintingVisitor :: visitPrintNode(PrintNode* node) {
	cout << "print(";
	for (unsigned int i = 0; i < node->operands(); ++i) {
		if (i != 0) cout<< ", ";
		AstNode* op = node->operandAt(i);
		op->visit(this);
	}
  cout << ");" << endl;
}
 
std::string PrintingVisitor::getType(VarType type )
{
  switch(type) {
	case VT_VOID: return "void";
    case VT_DOUBLE: return "double";
    case VT_INT: return "int";
    case VT_STRING: return "string";
    case VT_INVALID: return "";
  }
  return "";
}
