#include "AstShowVisitor.h"

#include <sstream>

using std::string;
using namespace mathvm;
string AstShowVisitor::getTypeName(mathvm::VarType type) {
	switch (type) {
		case VT_VOID:
			return "void";
			break;
		case VT_INT:
			return "int";
		case VT_DOUBLE:
			return "double";
		case VT_STRING:
			return "string";
		case VT_INVALID:
			return "";
	}
	return "";
}

void AstShowVisitor::visitBinaryOpNode(mathvm::BinaryOpNode* node) {
	_outputStream << '(';
	node->left()->visit(this);
	_outputStream << ' ';
	_outputStream << tokenOp(node->kind()) << ' ';
	node->right()->visit(this);
	_outputStream << ')';
}

void AstShowVisitor::visitUnaryOpNode(mathvm::UnaryOpNode* node) {
	_outputStream << "( " << tokenOp(node->kind());
	node->operand()->visit(this);
	_outputStream << " )";
}

void AstShowVisitor::visitStringLiteralNode(mathvm::StringLiteralNode* node) {
	std::string str  =  node->literal().c_str();
	_outputStream << "\'";
	for (std::string::iterator it=str.begin(); it < str.end(); ++it) {
		(*it == '\n') ? 
			_outputStream << "\\n" : _outputStream << *it; 
	}
	_outputStream << "\'";
}

void AstShowVisitor::visitDoubleLiteralNode(mathvm::DoubleLiteralNode* node) {
	std::ostringstream strs;
	strs << node->literal();
	std::string str = strs.str();
	
	size_t ePos = str.find("e+");
	if (ePos != string::npos) {
		str.replace(ePos, 2, "e");
	} 
	_outputStream << str;
}

void AstShowVisitor::visitIntLiteralNode(mathvm::IntLiteralNode* node) {
	_outputStream << node->literal();
}

void AstShowVisitor::visitLoadNode(mathvm::LoadNode* node) {
	_outputStream << node->var()->name();
}

void AstShowVisitor::visitStoreNode(mathvm::StoreNode* node) {
	_outputStream << node->var()->name() << " ";
	_outputStream << tokenOp(node->op()) << " ";
	node->value()->visit(this);	
	_outputStream << ";\n";															
}

void AstShowVisitor::visitForNode(mathvm::ForNode* node) {
	_outputStream << "for (" << node->var()->name() << " in ";
	node->inExpr()->visit(this);
	_outputStream << ") {\n";
	node->body()->visit(this);
	_outputStream << "\n}\n";
}

void AstShowVisitor::visitWhileNode(mathvm::WhileNode* node) {
	_outputStream << "while (";
	node->whileExpr()->visit(this);
	_outputStream << ") {\n";
	node->loopBlock()->visitChildren(this);
	_outputStream << "\n}\n";
}

 void AstShowVisitor::visitIfNode(mathvm::IfNode* node) {
	_outputStream << "if (";
	node->ifExpr()->visit(this);
	_outputStream << ") {\n";
	node->thenBlock()->visit(this);
	_outputStream << "\n}";
	if (node->elseBlock()) {
		_outputStream << " else {\n";
		node->elseBlock()->visit(this);
		_outputStream << "\n}";
	}
	_outputStream << "\n";
 }
 
 void AstShowVisitor::visitBlockNode(mathvm::BlockNode* node) {
	Scope::VarIterator varIt(node->scope());
	while(varIt.hasNext()) {
		AstVar* astVar = varIt.next();
		_outputStream << getTypeName(astVar->type()) << " "
			<< astVar->name() << ";\n";
	}
	node->visitChildren(this);
 }
 
 void AstShowVisitor::visitFunctionNode(mathvm::FunctionNode* node) {
	node->visitChildren(this);
 }
 
 void AstShowVisitor::visitPrintNode(mathvm::PrintNode* node) {
	_outputStream << "print(";
	int opCount = node->operands();
	for (int i = 0; i < opCount; ++i) {
		node->operandAt(i)->visit(this);
		if (i != opCount-1)
			_outputStream << ", ";
	}
	_outputStream << ");";
 }

void AstShowVisitor::visitReturnNode(mathvm::ReturnNode* node) {
    
    if (node->returnExpr()) {
        _outputStream << "return ";
        node->returnExpr()->visit(this);
    } else {
        _outputStream << "return";
    }
}

void AstShowVisitor::visitCallNode(mathvm::CallNode* node) {

    _outputStream << node->name() << '(';
    if (node->parametersNumber()) {
        node->parameterAt(0)->visit(this);
        for (uint32_t i = 1; i < node->parametersNumber(); ++i) {
            _outputStream << ", ";
            node->parameterAt(i)->visit(this);
        }
    }
    _outputStream << ')';
}

