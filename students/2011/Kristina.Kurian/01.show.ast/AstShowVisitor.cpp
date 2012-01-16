#include "AstShowVisitor.h"
#include <iostream>
#include <iomanip>
#include <sstream>
using namespace mathvm;
using std::cout;
using std::endl;
using std::string;

AstShowVisitor::AstShowVisitor() {}

void AstShowVisitor::visitBinaryOpNode(mathvm::BinaryOpNode* node) {
	cout << "(";
	node->left()->visit(this);
	cout << " " << tokenOp(node->kind()) << " ";
	node->right()->visit(this);
	cout << ")";
}

void AstShowVisitor::visitUnaryOpNode(mathvm::UnaryOpNode* node) {
	cout << "( " << tokenOp(node->kind());
	node->visitChildren(this);
	cout << ")";
}

void AstShowVisitor::visitStringLiteralNode(mathvm::StringLiteralNode* node) {
	cout << "\'";
	for(uint32_t i = 0; i < node->literal().size(); ++i) {
		char c = node->literal()[i];
		switch (c) {
			case '\n' :
				cout << "\\n";
				break;
			case '\b' :
				cout << "\\b";
				break;
			case '\t' :
				cout << "\\t";
				break;
			case '\a' :
				cout << "\\a";
				break;
			default:
				cout << c;
				break;
		}
	};
	cout << "\'";
}

void AstShowVisitor::visitDoubleLiteralNode(mathvm::DoubleLiteralNode* node) {
	std::stringstream str;
    	str << node->literal();
    	const std::string& s = str.str();
    	size_t r = s.find('e');
    	if (r != std::string::npos && r < s.length() - 1 && s[r + 1] == '+') {
        	cout << s.substr(0, r + 1) << s.substr(r + 2);
    	} else {
        	cout << node->literal();
        	if (r == std::string::npos && s.find('.') == std::string::npos) {
        	    cout << ".0";
        	}
    	}
//	cout << node->literal();
}

void AstShowVisitor::visitIntLiteralNode(mathvm::IntLiteralNode* node) {
	cout << node->literal();
}

void AstShowVisitor::visitLoadNode(mathvm::LoadNode* node) {
	cout << node->var()->name();
}

void AstShowVisitor::visitStoreNode(mathvm::StoreNode* node) {
	cout << node->var()->name() << " " << tokenOp(node->op()) << " ";
	node->visitChildren(this);
	cout << ";\n";
}

void AstShowVisitor::visitForNode( mathvm::ForNode* node ) {
	cout << "for (" << node->var()->name() << " in ";
	node->inExpr()->visit(this);
	cout << ") {\n";
	node->body()->visit(this);
	cout << "}\n";
}

void AstShowVisitor::visitWhileNode( mathvm::WhileNode* node ) {
	cout << "while (";
	node->whileExpr()->visit(this);
	cout << ") {\n";
	node->loopBlock()->visit(this);
	cout << "}\n";
}

void AstShowVisitor::visitIfNode( mathvm::IfNode* node ) {
	cout << "if (";
	node->ifExpr()->visit(this);
	cout << ") {\n";
	node->thenBlock()->visit(this);
	cout << "}";
	if (node->elseBlock()) {
		cout << " else {\n";
		node->elseBlock()->visit(this);
		cout << "\n}";
	} 
	cout << "\n";
}

void AstShowVisitor::visitBlockNode(mathvm::BlockNode* node) {
	Scope::FunctionIterator funcIt(node->scope());
	while(funcIt.hasNext()) {
		AstFunction *func = funcIt.next();
		cout << "function " << typeToName(func->returnType()) << " " << func->name() << "(";
		uint32_t count = func->parametersNumber();
		for (uint32_t i = 0; i < count; ++i) {
			cout << typeToName(func->parameterType(i)) << " " << func->parameterName(i);
			if (i != count - 1)
				cout << ", ";
		}
		cout << ") ";

		if (func->node()->body()->nodes() != 0 && func->node()->body()->nodeAt(0)->isNativeCallNode()) {
			func->node()->body()->nodeAt(0)->visit(this);
		}
		else {
			cout << "{\n";
			func->node()->visit(this);
			cout << "}\n\n";
		}
	}

	Scope::VarIterator it(node->scope());
	while(it.hasNext()) {
		AstVar *var = it.next();
		cout << typeToName(var->type()) << " " << var->name() << ";\n";
	}
	node->visitChildren(this);
}

void AstShowVisitor::visitFunctionNode(mathvm::FunctionNode* node) {
	node->visitChildren(this);
}

void AstShowVisitor::visitReturnNode( mathvm::ReturnNode* node ) {
	if (node->returnExpr()) {
        	cout << "return ";
        	node->visitChildren(this);
    	} else {
        	cout << "return";
    	}
	cout << ";\n";
}

void AstShowVisitor::visitPrintNode( mathvm::PrintNode* node ) {
	cout << "print(";
	for (uint32_t i = 0; i < node->operands(); ++i) {
		if (i != 0) {
			cout << ", ";
		}
		node->operandAt(i)->visit(this);
	}
	cout << ");\n";
}

void AstShowVisitor::visitCallNode( mathvm::CallNode* node ) {	
    	cout << node->name() << "(";
	if (node->parametersNumber()) {
		node->parameterAt(0)->visit(this);	
		for (uint32_t i = 1; i < node->parametersNumber(); ++i) {
			cout << ", ";
			node->parameterAt(i)->visit(this);
		}
	}
	cout << ")\n";
}

