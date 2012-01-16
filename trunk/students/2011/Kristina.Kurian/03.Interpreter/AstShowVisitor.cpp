#include "AstShowVisitor.h"
#include <iostream>
#include <iomanip>
#include <sstream>
using namespace mathvm;
using std::cout;
using std::endl;
using std::string;

AstShowVisitor::AstShowVisitor(std::ostream& o): stream(o) {}

void AstShowVisitor::visitBinaryOpNode(mathvm::BinaryOpNode* node) {
	stream << "(";
	node->left()->visit(this);
	stream << " " << tokenOp(node->kind()) << " ";
	node->right()->visit(this);
	stream << ")";
}

void AstShowVisitor::visitNativeCallNode( mathvm::NativeCallNode* node ) {
	stream << "native '" << node->nativeName() << "';\n\n";
}

void AstShowVisitor::visitUnaryOpNode(mathvm::UnaryOpNode* node) {
	stream << "( " << tokenOp(node->kind());
	node->visitChildren(this);
	stream << ")";
}

void AstShowVisitor::visitStringLiteralNode(mathvm::StringLiteralNode* node) {
	stream << "\'";
	for(uint32_t i = 0; i < node->literal().size(); ++i) {
		char c = node->literal()[i];
		switch (c) {
			case '\n' :
				stream << "\\n";
				break;
			case '\b' :
				stream << "\\b";
				break;
			case '\t' :
				stream << "\\t";
				break;
			case '\a' :
				stream << "\\a";
				break;
			default:
				stream << c;
				break;
		}
	};
	stream << "\'";
}

void AstShowVisitor::visitDoubleLiteralNode(mathvm::DoubleLiteralNode* node) {
	std::stringstream str;
    	str << node->literal();
    	const std::string& s = str.str();
    	size_t r = s.find('e');
    	if (r != std::string::npos && r < s.length() - 1 && s[r + 1] == '+') {
        	stream << s.substr(0, r + 1) << s.substr(r + 2);
    	} else {
        	stream << node->literal();
        	if (r == std::string::npos && s.find('.') == std::string::npos) {
        	    stream << ".0";
        	}
    	}
//	stream << node->literal();
}

void AstShowVisitor::visitIntLiteralNode(mathvm::IntLiteralNode* node) {
	stream << node->literal();
}

void AstShowVisitor::visitLoadNode(mathvm::LoadNode* node) {
	stream << node->var()->name();
}

void AstShowVisitor::visitStoreNode(mathvm::StoreNode* node) {
	stream << node->var()->name() << " " << tokenOp(node->op()) << " ";
	node->visitChildren(this);
	stream << ";\n";
}

void AstShowVisitor::visitForNode( mathvm::ForNode* node ) {
	stream << "for (" << node->var()->name() << " in ";
	node->inExpr()->visit(this);
	stream << ") {\n";
	node->body()->visit(this);
	stream << "}\n";
}

void AstShowVisitor::visitWhileNode( mathvm::WhileNode* node ) {
	stream << "while (";
	node->whileExpr()->visit(this);
	stream << ") {\n";
	node->loopBlock()->visit(this);
	stream << "}\n";
}

void AstShowVisitor::visitIfNode( mathvm::IfNode* node ) {
	stream << "if (";
	node->ifExpr()->visit(this);
	stream << ") {\n";
	node->thenBlock()->visit(this);
	stream << "}";
	if (node->elseBlock()) {
		stream << " else {\n";
		node->elseBlock()->visit(this);
		stream << "\n}";
	} 
	stream << "\n";
}

void AstShowVisitor::visitBlockNode(mathvm::BlockNode* node) {
	Scope::FunctionIterator funcIt(node->scope());
	while(funcIt.hasNext()) {
		AstFunction *func = funcIt.next();
		stream << "function " << typeToName(func->returnType()) << " " << func->name() << "(";
		uint32_t count = func->parametersNumber();
		for (uint32_t i = 0; i < count; ++i) {
			stream << typeToName(func->parameterType(i)) << " " << func->parameterName(i);
			if (i != count - 1)
				stream << ", ";
		}
		stream << ") ";

		if (func->node()->body()->nodes() != 0 && func->node()->body()->nodeAt(0)->isNativeCallNode()) {
			func->node()->body()->nodeAt(0)->visit(this);
		}
		else {
			stream << "{\n";
			func->node()->visit(this);
			stream << "}\n\n";
		}
	}

	Scope::VarIterator it(node->scope());
	while(it.hasNext()) {
		AstVar *var = it.next();
		stream << typeToName(var->type()) << " " << var->name() << ";\n";
	}
	node->visitChildren(this);
}

void AstShowVisitor::visitFunctionNode(mathvm::FunctionNode* node) {
	node->visitChildren(this);
}

void AstShowVisitor::visitReturnNode( mathvm::ReturnNode* node ) {
	if (node->returnExpr()) {
        	stream << "return ";
        	node->visitChildren(this);
    	} else {
        	stream << "return";
    	}
	stream << ";\n";
}

void AstShowVisitor::visitPrintNode( mathvm::PrintNode* node ) {
	stream << "print(";
	for (uint32_t i = 0; i < node->operands(); ++i) {
		if (i != 0) {
			stream << ", ";
		}
		node->operandAt(i)->visit(this);
	}
	stream << ");\n";
}

void AstShowVisitor::visitCallNode( mathvm::CallNode* node ) {	
    	stream << node->name() << "(";
	if (node->parametersNumber()) {
		node->parameterAt(0)->visit(this);	
		for (uint32_t i = 1; i < node->parametersNumber(); ++i) {
			stream << ", ";
			node->parameterAt(i)->visit(this);
		}
	}
	stream << ")\n";
}

