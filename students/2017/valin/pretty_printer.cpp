#include "ast.h"
#include "pretty_printer.h"

#define USING_NODES(type, name) using mathvm::type;
FOR_NODES(USING_NODES)
#undef USING_NODES

using mathvm::AstVar;
using mathvm::AstFunction;
using my::AstPrinter;

void AstPrinter::visitBinaryOpNode(BinaryOpNode * node)
{
	Braces braces(this);

	node->left()->visit(this);
	code << " " << tokenOp(node->kind()) << " ";
	node->right()->visit(this);
}

void AstPrinter::visitUnaryOpNode(UnaryOpNode * node)
{
	Braces braces(this);

	code << tokenOp(node->kind());
	node->visitChildren(this);
}

static std::string escape(const std::string& str)
{
	std::string _str = "'";
	for (auto c : str) {
		switch (c) {
		case '\n': _str += "\\n"; break;
		case '\t': _str += "\\t"; break;
		case '\\': _str += "\\\\"; break;
		case '\r': _str += "\\r"; break;
		default: _str.push_back(c);
		}
	}
	_str += "'";

	return _str;
}

void AstPrinter::visitStringLiteralNode(StringLiteralNode * node)
{
	code << escape(node->literal());
}

void AstPrinter::visitDoubleLiteralNode(DoubleLiteralNode * node)
{
	code << node->literal();
}

void AstPrinter::visitIntLiteralNode(IntLiteralNode * node)
{
	code << node->literal();
}

void AstPrinter::visitLoadNode(LoadNode * node)
{
	code << node->var()->name();
}

void AstPrinter::visitStoreNode(StoreNode * node)
{
	code << node->var()->name() << " " << tokenOp(node->op()) << " ";
	node->visitChildren(this);
	code << ";";
}

void AstPrinter::visitForNode(ForNode * node)
{
	code << "for ";

	{
		Braces braces(this);
		code << node->var()->name() << " in ";
		node->inExpr()->visit(this);
	}

	node->body()->visit(this);
}

void AstPrinter::visitWhileNode(WhileNode * node)
{
	code << "while";

	{
		Braces braces(this);
		node->whileExpr()->visit(this);
	}

	node->loopBlock()->visit(this);
}

void AstPrinter::visitIfNode(IfNode * node)
{
	code << "if ";

	{
		Braces braces(this);
		node->ifExpr()->visit(this);
	}

	code << " ";
	node->thenBlock()->visit(this);

	if (node->elseBlock()) {
		code << " else ";
		node->elseBlock()->visit(this);
	}
}

void AstPrinter::define(AstVar * var)
{
	code << typeToName(var->type()) << " " << var->name() << ";";
}

void AstPrinter::define(AstFunction * fun)
{
	code << "function " << typeToName(fun->returnType()) << " ";
	fun->node()->visit(this);
}

void AstPrinter::visitBlockNode(BlockNode * node)
{
	CurveBraces braces(this);
	Tab tab(this);

	mathvm::Scope::VarIterator i_var(node->scope(), false); 
	while (i_var.hasNext()) {
		Statement statement(this);
		AstVar * var = i_var.next();
		define(var);
	}

	mathvm::Scope::FunctionIterator i_fun(node->scope(), false); 
	while (i_fun.hasNext()) {
		Statement statement(this);
		AstFunction * fun = i_fun.next();
		define(fun);
	}

	for (size_t i = 0; i < node->nodes(); ++i) {
		Statement statement(this);
		node->nodeAt(i)->visit(this);
	}
}

void AstPrinter::visitFunctionNode(FunctionNode * node)
{
	code << node->name();

	{
		Braces braces(this);
		for (size_t i = 0; i < node->parametersNumber(); ++i) {
			code << typeToName(node->parameterType(i)) << ' ' << node->parameterName(i);
			if (i + 1 != node->parametersNumber()) {
				code << ", ";
			}
		}
	}

	code << " ";

	node->visitChildren(this);	
}

void AstPrinter::visitReturnNode(ReturnNode * node)
{
	code << "return";

	if (node->returnExpr()) {
		code << " ";
		node->visitChildren(this);
	}

	code << ";";
}

void AstPrinter::visitCallNode(CallNode * node)
{
	code << node->name();
	Braces braces(this);
       	for (size_t i = 0; i < node->parametersNumber(); ++i) {
		node->parameterAt(i)->visit(this);
		if (i + 1 != node->parametersNumber()) {
			code << ", ";
		}
	}
}

void AstPrinter::visitNativeCallNode(NativeCallNode * node)
{
	code << "[ NATIVE CALL ]";
}

void AstPrinter::visitPrintNode(PrintNode * node)
{
	code << "print";

	{
		Braces braces(this);

		for (size_t i = 0; i < node->operands(); ++i) {
			node->operandAt(i)->visit(this);
			if (i + 1 != node->operands()) {
				code << ", ";
			}
		}
	}

	code << ";";
}

void AstPrinter::pretty_printer(mathvm::AstFunction * root, std::ostream& code)
{
	AstPrinter printer(code);
	BlockNode * rootBlock = root->node()->body();

	mathvm::Scope::VarIterator i_var(rootBlock->scope(), false); 
	while (i_var.hasNext()) {
		Statement statement(&printer);
		AstVar * var = i_var.next();
		printer.define(var);
	}

	mathvm::Scope::FunctionIterator i_fun(rootBlock->scope(), false); 
	while (i_fun.hasNext()) {
		Statement statement(&printer);
		AstFunction * fun = i_fun.next();
		printer.define(fun);
	}

	for (size_t i = 0; i < rootBlock->nodes(); ++i) {
		Statement statement(&printer);
		rootBlock->nodeAt(i)->visit(&printer);
	}
}
