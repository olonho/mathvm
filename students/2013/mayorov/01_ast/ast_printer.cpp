#include "ast_printer.h"

void AstPrinter::startVisiting(AstFunction * top) {
	top->node()->body()->visit(this);
}

void AstPrinter::visitBlockNode(BlockNode *node) {
	Scope::VarIterator vars(node->scope());

	while (vars.hasNext()) {
		AstVar* var = vars.next();
		_out << typeToName(var->type()) << " " << var->name() << ";" << std::endl;
	}

	Scope::FunctionIterator funcs(node->scope());

	while (funcs.hasNext()) {
		funcs.next()->node()->visit(this);
	}

	node->visitChildren(this);
};

void AstPrinter::visitStoreNode(StoreNode *node) {
	_out << node->var()->name() << " ";
	_out << tokenOp(node->op()) << " ";
	node->value()->visit(this);
	_out << ";" << std::endl;
};

void AstPrinter::visitIntLiteralNode(IntLiteralNode *node) {
	_out << node->literal();
};

void AstPrinter::visitDoubleLiteralNode(DoubleLiteralNode *node) {
	_out << node->literal();
};

void AstPrinter::visitStringLiteralNode(StringLiteralNode *node) {

	_out << "\'";

	for (std::size_t i = 0; i < node->literal().size(); ++i)
		switch(node->literal()[i])
		{
		case '\n':
			_out << "\\n";  break;
		case '\t':
			_out << "\\t";  break;
		case '\r':
			_out << "\\r";  break;
		case '\'':
			_out << "\\'";  break;
		case '\\':
			_out << "\\\\"; break;
		default:
			_out << node->literal()[i] ;
		}

	_out << "\'";
};


void AstPrinter::visitBinaryOpNode(BinaryOpNode *node) {
	_out << "(";
	node->left()->visit(this);
	_out << " " << tokenOp(node->kind()) << " ";
	node->right()->visit(this);
	_out << ")";
}

void AstPrinter::visitUnaryOpNode(UnaryOpNode* node) {
	_out << tokenOp(node->kind());
	node->operand()->visit(this);
}

void AstPrinter::visitPrintNode(PrintNode *node) {
	_out << "print(";
	for (uint32_t i = 0; i != node->operands(); ++i)
	{
		node->operandAt(i)->visit(this);
		if (i != node->operands() - 1)
			_out << ", ";
	}
	_out << ");" << std::endl;
};

void AstPrinter::visitLoadNode(LoadNode *node) {
	_out << node->var()->name();
};

void AstPrinter::visitForNode(ForNode *node) {
	_out << "for (" << node->var()->name() << " in ";
	node->inExpr()->visit(this);
	_out << ") {" << std::endl;
	node->body()->visit(this);
	_out << "}" << std::endl;
};

void AstPrinter::visitWhileNode(WhileNode *node) {
	_out << "while (";
	node->whileExpr()->visit(this);
	_out << ") {" << std::endl;
	node->loopBlock()->visit(this);
	_out << "}" << std::endl;
};

void AstPrinter::visitIfNode(IfNode *node) {
	_out << "if (";
	node->ifExpr()->visit(this);
	_out << ") {" << std::endl;
	node->thenBlock()->visit(this);

	if (node->elseBlock() != 0) {
		_out << "} else {" << std::endl;
		node->elseBlock()->visit(this);
	}
	_out << "}" << std::endl;

};

void AstPrinter::visitFunctionNode(FunctionNode *node) {

	_out << "function " << typeToName(node->returnType()) << " ";
	_out << node->name() << "(";
	for(uint32_t i = 0; i < node->parametersNumber(); ++i) {
		_out << typeToName(node->parameterType(i)) << " " << node->parameterName(i);
		if (i != node->parametersNumber() - 1) {
			_out << ", ";
		}
	}
	_out << ")";

	if (node->body()->nodes() && node->body()->nodeAt(0)->isNativeCallNode()) {
		//native mode
		_out << " ";
		node->body()->nodeAt(0)->visit(this);
		_out << ";" << std::endl;
	} else {
		_out << "{" << std::endl;
		node->body()->visit(this);
		_out << "}" << std::endl;
	}

};

void AstPrinter::visitReturnNode(ReturnNode *node) {

	if (node->returnExpr() == 0) {
		return;
	}

	_out << "return ";
	node->returnExpr()->visit(this);
	_out << ";" << std::endl;
};

void AstPrinter::visitCallNode(CallNode *node) {
	_out << node->name() << "(";
	for(uint32_t i = 0; i < node->parametersNumber(); ++i) {
		node->parameterAt(i)->visit(this);
		if (i != node->parametersNumber() - 1) {
			_out << ", ";
		}
	}
	_out << ")" << std::endl;
};

void AstPrinter::visitNativeCallNode(NativeCallNode *node) {
	_out << "native '" << node->nativeName() << "';";
};
