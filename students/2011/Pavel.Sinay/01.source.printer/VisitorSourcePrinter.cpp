/*
 * VisitorSourcePrinter.cpp
 *
 *  Created on: 23.10.2011
 *      Author: niea
 */

#include "VisitorSourcePrinter.h"

VisitorSourcePrinter::VisitorSourcePrinter(std::ostream &msg_stream) :
	m_stream(msg_stream) {
	// TODO Auto-generated constructor stub
}

VisitorSourcePrinter::~VisitorSourcePrinter() {
	// TODO Auto-generated destructor stub
}

void VisitorSourcePrinter::visitIfNode(mathvm::IfNode *node) {
	//m_stream << "<if node> ";
	m_stream << "if (";
	node->ifExpr()->visit(this);
	m_stream << ") {" << std::endl;
	node->thenBlock()->visit(this);

	if (node->elseBlock()) {
		m_stream << std::endl << "} else {" << std::endl;
		node->elseBlock()->visit(this);
	}
	m_stream << "}" << std::endl;
}

void VisitorSourcePrinter::visitPrintNode(mathvm::PrintNode *node) {
	//m_stream << "print node" << std::endl;
	m_stream << "print (";
	//node->visitChildren(this);
	for (unsigned int i = 0; i < node->operands(); ++i) {
		node->operandAt(i)->visit(this);
		if (i != node->operands() - 1) {
			m_stream << ", ";
		}
	}

	m_stream << ");" << std::endl;
}

void VisitorSourcePrinter::visitLoadNode(mathvm::LoadNode *node) {
	//m_stream << "<load node> ";
	m_stream << node->var()->name();
}

void VisitorSourcePrinter::visitForNode(mathvm::ForNode *node) {
	//m_stream << "<for node> ";
	m_stream << "for (" << node->var()->name() << " in ";
	node->inExpr()->visit(this);
	m_stream << ") {" << std::endl;
	node->body()->visit(this);
	m_stream << "}" << std::endl;
}

void VisitorSourcePrinter::visitIntLiteralNode(mathvm::IntLiteralNode *node) {
	//m_stream << "<IntLiteral node> ";
	m_stream << node->literal();
}

void VisitorSourcePrinter::visitUnaryOpNode(mathvm::UnaryOpNode *node) {
	//m_stream << "<unary op node> ";
	m_stream << tokenOp(node->kind());
	node->operand()->visit(this);
}

void VisitorSourcePrinter::visitFunctionNode(mathvm::FunctionNode *node) {
	//m_stream << "<function node> " << std::endl;
	m_stream << "function "  << varTypeToStr(node->returnType())
			<< " " << node->name() << " (";

	m_stream << ") {" << std::endl;
	//node->body() parametersNumber()
	node->body()->visit(this);
	//node->visitChildren(this);
	m_stream << "}" << std::endl;
}

void VisitorSourcePrinter::visitWhileNode(mathvm::WhileNode *node) {
	//m_stream << "<while node> ";
	m_stream << "while (";
	node->whileExpr()->visit(this);
	m_stream << ") {" << std::endl;
	node->loopBlock()->visit(this);
	m_stream << std::endl << "}" << std::endl;
}

void VisitorSourcePrinter::visitBinaryOpNode(mathvm::BinaryOpNode *node) {
	//m_stream << "<binary op node> ";
	m_stream << "(";
	node->left()->visit(this);
	m_stream << tokenOp(node->kind());
	node->right()->visit(this);
	m_stream << ")";
}

void VisitorSourcePrinter::visitBlockNode(mathvm::BlockNode *node) {
	//m_stream << "<block node> " << std::endl;
	mathvm::Scope::VarIterator it(node->scope());
	while (it.hasNext()) {
		mathvm::AstVar *var = it.next();
		m_stream << varTypeToStr(var->type()) << " " << var->name() << ";"
				<< std::endl;
	}

	mathvm::Scope::FunctionIterator f_it(node->scope());
	while (f_it.hasNext()) {
		f_it.next()->node()->visit(this);
	}

	node->visitChildren(this);
}

void VisitorSourcePrinter::visitDoubleLiteralNode(
		mathvm::DoubleLiteralNode *node) {
	//m_stream << "<double literal node> ";
	m_stream << node->literal();
}

void VisitorSourcePrinter::visitStringLiteralNode(
		mathvm::StringLiteralNode *node) {
	//m_stream << "<string literal node> ";
	std::string str = node->literal();
	int n_pos;
	while ((n_pos = str.find('\n')) >= 0) {
		str.erase(n_pos, 1);
		str.insert(n_pos, "\\n");
	}
	m_stream << "'" << str << "'";
}

void VisitorSourcePrinter::visitStoreNode(mathvm::StoreNode *node) {
	//m_stream << "<store node> ";
	//node->visitChildren(this);
	m_stream << node->var()->name() << tokenOp(node->op());
	node->value()->visit(this);
	m_stream << ';' << std::endl;
}

void VisitorSourcePrinter::visitCallNode(mathvm::CallNode *node) {
	m_stream << node->name() << "(";
	for (uint32_t i = 0; i != node->parametersNumber(); ++i) {
		node->parameterAt(i)->visit(this);
		if (i != node->parametersNumber() - 1) {
			m_stream << ", ";
		}
	}
	m_stream << ")";
}

std::string VisitorSourcePrinter::varTypeToStr(mathvm::VarType type) {
	switch (type) {
	case mathvm::VT_INVALID:
		return "<invalid var type>";
	case mathvm::VT_VOID:
		return "void";
	case mathvm::VT_DOUBLE:
		return "double";
	case mathvm::VT_INT:
		return "int";
	case mathvm::VT_STRING:
		return "string";
	}
	return "";
}

