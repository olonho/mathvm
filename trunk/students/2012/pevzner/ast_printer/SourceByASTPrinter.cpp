/*
 * SourceByASTPrinter.cpp
 *
 *  Created on: 28.09.2012
 *      Author: alina
 */

#include "SourceByASTPrinter.h"

using std::cout;
using std::endl;

namespace mathvm {

SourceByASTPrinter::SourceByASTPrinter() {

}

SourceByASTPrinter::~SourceByASTPrinter() {

}

void SourceByASTPrinter::performPrint(AstFunction *top){
	if(top == 0 || top->node() == 0){
		cout<<"AST is empty!"<<endl;
		return;
	}

	visitBlockNodeWithoutBraces(top->node()->body());
}

void SourceByASTPrinter::visitBinaryOpNode(BinaryOpNode *node){
	node->left()->visit(this);
	cout<<" "<<tokenOp(node->kind())<<" ";
	node->right()->visit(this);
}

void SourceByASTPrinter::visitUnaryOpNode(UnaryOpNode *node){
	cout<<tokenOp(node->kind());
	node->operand()->visit(this);
}

void SourceByASTPrinter::visitStringLiteralNode(StringLiteralNode *node){
	string ltrl = node->literal();
	string rslt = "";
	for(unsigned int i = 0; i < ltrl.size(); ++i){
		switch(ltrl[i]){
		case '\n': rslt += "\\n";   break;
		case '\t': rslt += "\\t";   break;
		case '\r': rslt += "\\r";   break;
		case '\\': rslt += "\\\\";  break;
		default:   rslt += ltrl[i]; break;
		}
	}

	cout<<"'"<<rslt<<"'";
}

void SourceByASTPrinter::visitIntLiteralNode(IntLiteralNode *node){
	cout<<node->literal();
}

void SourceByASTPrinter::visitDoubleLiteralNode(DoubleLiteralNode *node){
	cout<<node->literal();
}

void SourceByASTPrinter::visitLoadNode(LoadNode *node){
	cout<<node->var()->name();
}

void SourceByASTPrinter::visitStoreNode(StoreNode *node){
	cout<<node->var()->name()<<" "<<tokenOp(node->op())<<" ";
	node->value()->visit(this);
}

void SourceByASTPrinter::visitBlockNodeWithoutBraces(BlockNode *node){
	Scope::VarIterator vIt(node->scope());
	while(vIt.hasNext()) {
		AstVar *var = vIt.next();
		cout<<typeToName(var->type())<<" "<< var->name()<< ";"<<endl;
	}

	Scope::FunctionIterator fIt(node->scope());
	while(fIt.hasNext()) {
		fIt.next()->node()->visit(this);
	}

	int nodesNumber = node->nodes();
	for (int i = 0; i < nodesNumber; ++i) {
		AstNode *nodeAt = node->nodeAt(i);
		nodeAt->visit(this);
		if(!nodeAt->isForNode() && !nodeAt->isIfNode() && !nodeAt->isWhileNode())
			cout<<";"<<endl;
	}
}

void SourceByASTPrinter::visitBlockNode(BlockNode *node){
	cout<<"{"<<endl;
	visitBlockNodeWithoutBraces(node);
	cout<<"}"<<endl;
}

void SourceByASTPrinter::visitNativeCallNode(NativeCallNode *node){
	cout<<" native '"<<node->nativeName()<<"';";
}

void SourceByASTPrinter::visitForNode(ForNode *node){
	cout<< "for ("<<node->var()->name()<<" in ";
	node->inExpr()->visit(this);
	cout<< ") ";
	node->body()->visit(this);
}

void SourceByASTPrinter::visitWhileNode(WhileNode *node){
	cout<< "while (";
	node->whileExpr()->visit(this);
	cout<< ") ";
	node->loopBlock()->visit(this);
}

void SourceByASTPrinter::visitIfNode(IfNode *node){
	cout<< "if (";
	node->ifExpr()->visit(this);
	cout<< ") ";
	node->thenBlock()->visit(this);
	if(node->elseBlock()) {
		cout<<"else ";
	    node->elseBlock()->visit(this);
	}
}

void SourceByASTPrinter::visitReturnNode(ReturnNode *node){
	cout<<"return ";
    node->visitChildren(this);
}

void SourceByASTPrinter::visitFunctionNode(FunctionNode *node){
	cout<<"function "<<typeToName(node->returnType())<<" "<< node->name()<<"(";

	int paramNumber = node->parametersNumber();
	for (int i = 0; i < paramNumber; ++i) {
		cout<<typeToName(node->parameterType(i))<<" "<<node->parameterName(i);
		if(i < paramNumber - 1) cout<<",";
	}
	cout<<")";

	if (node->body()->nodeAt(0)->isNativeCallNode()) {
	    node->body()->nodeAt(0)->visit(this);
	    cout<< std::endl;
	} else {
		node->body()->visit(this);
	}
}

void SourceByASTPrinter::visitCallNode(CallNode *node){
	cout<<node->name()<<"(";
	int parametersNumber = node->parametersNumber();
	for (int i = 0; i < parametersNumber; ++i){
		node->parameterAt(i)->visit(this);
		if(i < parametersNumber - 1) cout<<", ";
	}
	cout<<")";
}

void SourceByASTPrinter::visitPrintNode(PrintNode *node){
	cout<<"print (" ;
	int operandsNumber = node->operands();
	for (int i = 0; i < operandsNumber; ++i) {
		node->operandAt(i)->visit(this);
		if(i < operandsNumber - 1) cout<<", ";
	}
	cout<<")";
}

}
