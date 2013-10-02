#include "AstPrinter.h"

AstPrinter::AstPrinter(std::ostream& out)
: mathvm::AstVisitor()
, indent(0)
, out(out)
{}


void AstPrinter::printVarType(mathvm::VarType type) {
	switch (type) {
	case mathvm::VT_VOID:
		out << "void";
		break;
	case mathvm::VT_DOUBLE:
		out << "double";
		break;
	case mathvm::VT_INT:
		out << "int";
		break;
	case mathvm::VT_STRING:
		out << "string";
		break;
	default:
		// TODO: error handling
		out << "fixme! unknown/unsupported type";
		break;
	}
}

void AstPrinter::visitBinaryOpNode(mathvm::BinaryOpNode* node)
{
	mathvm::AstNode* left = node->left();
	mathvm::AstNode* rigth = node->right();
	mathvm::TokenKind op = node->kind();

	out << "(";
	left->visit(this);
	out << ") ";

	switch (op) {
#define OP_CASE(token, str, prior) \
	case mathvm::token: \
		out << str; \
		break;

	FOR_TOKENS(OP_CASE)
#undef OP_CASE
	default:
		// TODO: error handling
		out << "fixme! unknown binary operation";
	}

	out << " (";
	rigth->visit(this);
	out << ")";
}

void AstPrinter::visitUnaryOpNode(mathvm::UnaryOpNode* node)
{
	mathvm::TokenKind op = node->kind();
	mathvm::AstNode* operand = node->operand();

	switch (op) {
	case mathvm::tNOT:
		out << "!";
		break;
	case mathvm::tSUB:
		out << "-";
		break;
	default:
		// TODO: error handling
		out << "fixme! unknown unary operator";
	}

	out << "(";
	operand->visit(this);
	out << ")";
}

void AstPrinter::escape(std::string& s)
{
	replaceAll(s, "\\", "\\\\");
	replaceAll(s, "\n", "\\n");
	replaceAll(s, "\r", "\\r");
	replaceAll(s, "\t", "\\t");
}

void AstPrinter::replaceAll(std::string& s, const std::string& x, const std::string& y)
{
	size_t pos = 0;
	while ((pos = s.find(x, pos)) != std::string::npos) {
		s.replace(pos, x.length(), y);
		pos += y.length();
	}
}

void AstPrinter::visitStringLiteralNode(mathvm::StringLiteralNode* node)
{
	std::string literal(node->literal());
	escape(literal);
	out << "'" << literal << "'";
}

void AstPrinter::visitDoubleLiteralNode(mathvm::DoubleLiteralNode* node)
{
	out << node->literal();
}

void AstPrinter::visitIntLiteralNode(mathvm::IntLiteralNode* node)
{
	out << node->literal();
}

void AstPrinter::visitLoadNode(mathvm::LoadNode* node)
{
	out << node->var()->name();
}

void AstPrinter::visitStoreNode(mathvm::StoreNode* node)
{
	const mathvm::AstVar* var = node->var();
	mathvm::AstNode* value = node->value();
	mathvm::TokenKind op = node->op();

	out << var->name() << " ";

	switch (op) {
	case mathvm::tASSIGN:
		out << "=";
		break;
	case mathvm::tINCRSET:
		out << "+=";
		break;
	case mathvm::tDECRSET:
		out << "-=";
		break;
	default:
		// TODO: error handling
		out << "fixme! unsupported assing operation";
	}

	out << " ";

	value->visit(this);
}

void AstPrinter::visitForNode(mathvm::ForNode* node)
{
	const mathvm::AstVar* var = node->var();
	mathvm::AstNode* expr = node->inExpr();
	mathvm::BlockNode* body = node->body();

	out << "for (" << var->name() << " in ";
	expr->visit(this);
	out << ")" << std::endl;
	body->visit(this);
}

void AstPrinter::visitWhileNode(mathvm::WhileNode* node)
{
	mathvm::AstNode* expr = node->whileExpr();
	mathvm::BlockNode* body = node->loopBlock();

	out << "while (";
	expr->visit(this);
	out << ")" << std::endl;
	body->visit(this);
}

void AstPrinter::visitIfNode(mathvm::IfNode* node)
{
	mathvm::AstNode* expr = node->ifExpr();
	mathvm::BlockNode* thenBody = node->thenBlock();
	mathvm::BlockNode* elseBody = node->elseBlock();

	out << "if (";
	expr->visit(this);
	out << ")" << std::endl;
	thenBody->visit(this);

	if (elseBody) {
		printIndent();
		out << "else" << std::endl;
		elseBody->visit(this);
	}
}

void AstPrinter::printBlockContent(mathvm::BlockNode* node)
{
	mathvm::Scope* scope = node->scope();

	mathvm::Scope::VarIterator vars(scope);
	while (vars.hasNext()) {
		mathvm::AstVar* var = vars.next();

		printIndent();
		printVarType(var->type());
		out << " " << var->name() << ";" << std::endl;
	}

	mathvm::Scope::FunctionIterator fns(scope);
	while (fns.hasNext()) {
		printIndent();
		fns.next()->node()->visit(this);
	}

	uint32_t count = node->nodes();
	for (uint32_t i = 0; i < count; ++i) {
		mathvm::AstNode* child = node->nodeAt(i);

		printIndent();
		child->visit(this);

		if (!child->isBlockNode() && !child->isForNode() &&
		    !child->isWhileNode() && !child->isIfNode()) {
			out << ";" << std::endl;
		}
	}
}

void AstPrinter::visitBlockNode(mathvm::BlockNode* node)
{
	printIndent();
	out << "{" << std::endl;
	++indent;

	printBlockContent(node);

	--indent;
	printIndent();
	out << "}" << std::endl;
}

void AstPrinter::visitFunctionNode(mathvm::FunctionNode* node)
{
	out << "function ";
	printVarType(node->returnType());
	out << " " << node->name() << "(";

	uint32_t count = node->parametersNumber();
	for (uint32_t i = 0; i < count; ++i) {
		if (i > 0) out << ", ";
		printVarType(node->parameterType(i));
		out << " " << node->parameterName(i);
	}

	out << ")";

	bool isNative = node->body()->nodes() &&
	                node->body()->nodeAt(0)->isNativeCallNode();

	if (isNative) {
		out << " native '";
		out << node->body()->nodeAt(0)->asNativeCallNode()->nativeName();
		out << "';" << std::endl;
	} else {
		out << std::endl;
		node->body()->visit(this);
	}
}

void AstPrinter::visitReturnNode(mathvm::ReturnNode* node)
{
	out << "return";

	if (node->returnExpr()) {
		out << " ";
		node->returnExpr()->visit(this);
	}
}

void AstPrinter::visitCallNode(mathvm::CallNode* node)
{
	out << node->name() << "(";

	uint32_t count = node->parametersNumber();
	for (uint32_t i = 0; i < count; ++i) {
		if (i > 0) out << ", ";
		node->parameterAt(i)->visit(this);
	}

	out << ")";
}

void AstPrinter::visitNativeCallNode(mathvm::NativeCallNode* node)
{
	// Native functions should be wrapped via functions with special syntax
	// so there are no direct native calls in the source code
	out << "native call node" << std::endl;
}

void AstPrinter::visitPrintNode(mathvm::PrintNode* node)
{
	uint32_t count = node->operands();

	out << "print(";

	for (uint32_t i = 0; i < count; ++i) {
		if (i > 0) out << ", ";
		mathvm::AstNode* operand = node->operandAt(i);
		operand->visit(this);
	}

	out << ")";
}


void AstPrinter::printIndent()
{
	for (int i = 0; i < indent; ++i) {
		out << "\t";
	}
}
