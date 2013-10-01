#include "AstPrinter.h"

AstPrinter::AstPrinter(std::ostream& out)
: mathvm::AstVisitor()
, indent(0)
, out(out)
{}

void AstPrinter::printAst(mathvm::AstFunction* topfn)
{
	mathvm::BlockNode* topnode = topfn->node()->body();
	mathvm::Scope* topscope = topnode->scope();

	mathvm::Scope::FunctionIterator fns(topscope);
	while (fns.hasNext()) {
		printFunc(fns.next());
	}

	mathvm::Scope::VarIterator vars(topscope);
	while (vars.hasNext()) {
		printVarDecl(vars.next());
	}

	topnode->visitChildren(this);
}

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
		out << "fixme!";
		break;
	}
}

void AstPrinter::printVarDecl(mathvm::AstVar* var)
{
	printIndent();
	printVarType(var->type());
	out << " " << var->name() << ";" << std::endl;
}


void AstPrinter::printFunc(mathvm::AstFunction* fn)
{
	printIndent();
}

void AstPrinter::visitBinaryOpNode(mathvm::BinaryOpNode* node)
{
}

void AstPrinter::visitUnaryOpNode(mathvm::UnaryOpNode* node)
{
}

void AstPrinter::visitStringLiteralNode(mathvm::StringLiteralNode* node)
{
}

void AstPrinter::visitDoubleLiteralNode(mathvm::DoubleLiteralNode* node)
{
}

void AstPrinter::visitIntLiteralNode(mathvm::IntLiteralNode* node)
{
}

void AstPrinter::visitLoadNode(mathvm::LoadNode* node)
{
}

void AstPrinter::visitStoreNode(mathvm::StoreNode* node)
{
}

void AstPrinter::visitForNode(mathvm::ForNode* node)
{
}

void AstPrinter::visitWhileNode(mathvm::WhileNode* node)
{
}

void AstPrinter::visitIfNode(mathvm::IfNode* node)
{
}

void AstPrinter::visitBlockNode(mathvm::BlockNode* node)
{
}

void AstPrinter::visitFunctionNode(mathvm::FunctionNode* node)
{
}

void AstPrinter::visitReturnNode(mathvm::ReturnNode* node)
{
}

void AstPrinter::visitCallNode(mathvm::CallNode* node)
{
}

void AstPrinter::visitNativeCallNode(mathvm::NativeCallNode* node)
{
}

void AstPrinter::visitPrintNode(mathvm::PrintNode* node)
{
}


void AstPrinter::printIndent()
{
	for (int i = 0; i < indent; ++i) {
		out << "\t";
	}
}
