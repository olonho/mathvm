#include "printer.h"
#include <cstdio>

using namespace mathvm;

AstPrinterStyle AstPrinter::testStyle() {
	AstPrinterStyle result;
	result.forceBlockBraces = true;
	result.forceExpressionBraces = true;
	result.splitFunctions = false;
	result.scopeIndent = "  ";
	result.funcBodyIndent = "  ";
	return result;
};

AstPrinterStyle AstPrinter::prettyStyle() {
	return AstPrinterStyle();
}


AstPrinter::AstPrinter(AstPrinterStyle const& style):
	style(style)
{}

string AstPrinter::print(AstNode *root) {
	wasStatement = false;
	indents.clear();
	ss.clear();
	if (root && root->isFunctionNode())
		visitBlock(root->asFunctionNode()->body(), true);
	return ss.str();
}

void AstPrinter::indent() {
	for (auto const &s: indents)
		ss << s;
}

void AstPrinter::printType(VarType type) {
	ss << typeToName(type);
	wasStatement = false;
}

void AstPrinter::printVar(AstVar const *var) {
	printType(var->type());
	ss << " " << var->name();
	wasStatement = false;
}

void AstPrinter::visitExpression(AstNode *expr, bool braced) {
	braced |= (
			style.forceExpressionBraces && (
				expr->isBinaryOpNode()
//				|| expr->isUnaryOpNode()
			)
	);
	if (braced) ss << "(";
	expr->visit(this);
	if (braced) ss << ")";
	wasStatement = false;
}

void AstPrinter::visitBlock(BlockNode *node, bool unbraced) {
	bool braced{style.forceBlockBraces};
	braced |= node->scope()->variablesCount() > 0;
	braced |= node->scope()->functionsCount() > 0;
	braced |= node->nodes() != 1;
	braced &= !unbraced;

	if (braced) {
		indents.push_back(style.scopeIndent);
		ss << "{\n";
	}

	for (Scope::VarIterator it(node->scope()); it.hasNext(); ) {
		auto var{it.next()};
		indent();
		printVar(var);
		ss << ";\n";
	}

	if (node->scope()->functionsCount() > 0 && style.splitFunctions)
		ss << "\n";
	for (Scope::FunctionIterator it(node->scope()); it.hasNext(); ) {
		auto fun{it.next()};
		indent();
		fun->node()->visit(this);
		if (style.splitFunctions)
			ss << "\n";
	}

	for (uint32_t i{0}; i != node->nodes(); ++i) {
		auto child(node->nodeAt(i));
		if (braced)
			indent();
		child->visit(this);
		if (!wasStatement) {
			ss << ";\n";
		}
	}

	if (braced) {
		indents.pop_back();
		indent();
		ss << "}\n";
	}
	wasStatement = true;
}

void AstPrinter::visitBinaryOpNode(BinaryOpNode *node) {
	int prec{tokenPrecedence(node->kind())};
	auto printOperand{[=](AstNode *operand) {
		bool innerPrec{99};
		if (operand->isUnaryOpNode()) {
			innerPrec = tokenPrecedence(operand->asUnaryOpNode()->kind());
		} else if (operand->isBinaryOpNode()) {
			innerPrec = tokenPrecedence(operand->asBinaryOpNode()->kind());
		}
		visitExpression(operand, innerPrec > prec);
	}};
	printOperand(node->left() );
	ss << " " << tokenOp(node->kind()) << " ";
	printOperand(node->right());
	wasStatement = false;
}

void AstPrinter::visitUnaryOpNode(UnaryOpNode *node) {
	ss << tokenOp(node->kind());
	visitExpression(node->operand(), node->operand()->isUnaryOpNode() || node->operand()->isBinaryOpNode());
	wasStatement = false;
}

void AstPrinter::visitStringLiteralNode(StringLiteralNode *node) {
	ss << "'";
	for (auto c: node->literal()) {
		switch (c) {
			case '\'':
				ss << "\\'";
				break;
			case '\a':
				ss << "\\a";
				break;
			case '\b':
				ss << "\\b";
				break;
			case '\f':
				ss << "\\f";
				break;
			case '\n':
				ss << "\\n";
				break;
			case '\r':
				ss << "\\r";
				break;
			case '\t':
				ss << "\\t";
				break;
			case '\v':
				ss << "\\v";
				break;
			default:
				ss << string(1, c);
		}
	}
	ss << "'";
	wasStatement = false;
}

void AstPrinter::visitDoubleLiteralNode(DoubleLiteralNode *node) {
	ss << node->literal();
	wasStatement = false;
}

void AstPrinter::visitIntLiteralNode(IntLiteralNode *node) {
	ss << node->literal();
	wasStatement = false;
}

void AstPrinter::visitLoadNode(LoadNode *node) {
	ss << node->var()->name();
	wasStatement = false;
}

void AstPrinter::visitStoreNode(StoreNode *node) {
	ss << node->var()->name() << " " << tokenOp(node->op()) << " ";
	visitExpression(node->value());
	wasStatement = false;
}

void AstPrinter::visitForNode(ForNode *node) {
	ss << "for (";
	printVar(node->var());
	ss << " in ";
	visitExpression(node->inExpr());
	ss << ") ";
	node->body()->visit(this);
	wasStatement = true;
}

void AstPrinter::visitWhileNode(WhileNode *node) {
	ss << "while (";
	visitExpression(node->whileExpr());
	ss << ") ";
	node->loopBlock()->visit(this);
	wasStatement = true;
}

void AstPrinter::visitIfNode(IfNode *node) {
	ss << "if (";
	visitExpression(node->ifExpr());
	ss << ") ";
	node->thenBlock()->visit(this);
	if (node->elseBlock()) {
		indent();
		ss << "else ";
		node->elseBlock()->visit(this);
	}
	wasStatement = true;
}

void AstPrinter::visitBlockNode(BlockNode *node) {
	visitBlock(node);
}

void AstPrinter::visitFunctionNode(FunctionNode *node) {
	ss << "function ";
	printType(node->returnType());
	ss << " " << node->name() << "(";
	for (uint32_t i{0}; i != node->parametersNumber(); ++i) {
		if (i != 0)
			ss << ", ";
		printType(node->parameterType(i));
		ss << " " << node->parameterName(i);
	}
	ss << ") ";
	indents.push_back(style.funcBodyIndent);
	node->body()->visit(this);
	indents.pop_back();
}

void AstPrinter::visitReturnNode(ReturnNode *node) {
	ss << "return ";
	visitExpression(node->returnExpr());
	wasStatement = false;
}

void AstPrinter::visitCallNode(CallNode *node) {
	ss << node->name() << "(";
	for (uint32_t i{0}; i != node->parametersNumber(); ++i) {
		if (i != 0)
			ss << ", ";
		visitExpression(node->parameterAt(i));
	}
	ss << ")";
	wasStatement = false;
}

void AstPrinter::visitNativeCallNode(NativeCallNode *node) {
	ss << "native '" << node->nativeName() << "'";
	wasStatement = false;
}

void AstPrinter::visitPrintNode(PrintNode *node) {
	ss << "print(";
	for (uint32_t i{0}; i != node->operands(); ++i) {
		if (i != 0)
			ss << ", ";
		node->operandAt(i)->visit(this);
	}
	ss << ")";
	wasStatement = false;
}

