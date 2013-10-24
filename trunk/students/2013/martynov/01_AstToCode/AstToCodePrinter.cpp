/*
 * AstToCodePrinter.cpp
 *
 *  Created on: Oct 10, 2013
 *      Author: Semen Martynov
 */

#include <stddef.h>
#include "AstToCodePrinter.h"

namespace mathvm
{
/*
 AstToCodePrinter::AstToCodePrinter()
 {
 // TODO Auto-generated constructor stub

 }

 AstToCodePrinter::~AstToCodePrinter()
 {
 // TODO Auto-generated destructor stub
 }
 */

void AstToCodePrinter::exec(AstFunction* top)
{
	processBlockNode(top->node()->body());
}

void AstToCodePrinter::processBlockNode(BlockNode * node)
{
	Scope::VarIterator vars(node->scope());
	while (vars.hasNext())
	{
		AstVar *var = vars.next();
		_stream << typeToName(var->type()) << " " << var->name() << ";\n";
	}

	Scope::FunctionIterator funs(node->scope());
	while (funs.hasNext())
	{
		funs.next()->node()->visit(this);
		_stream << "\n";
	}

	for (uint32_t i = 0; i != node->nodes(); ++i)
	{
		node->nodeAt(i)->visit(this);
		if (!node->nodeAt(i)->isForNode() && !node->nodeAt(i)->isWhileNode()
				&& !node->nodeAt(i)->isIfNode())
			_stream << ";\n";
	}
}

void AstToCodePrinter::visitBinaryOpNode(BinaryOpNode* node)
{
	_stream << "(";
	node->left()->visit(this);
	_stream << tokenOp(node->kind());
	node->right()->visit(this);
	_stream << ")";
}

void AstToCodePrinter::visitUnaryOpNode(UnaryOpNode* node)
{
	_stream << tokenOp(node->kind());
	node->operand()->visit(this);
}

void AstToCodePrinter::visitStringLiteralNode(StringLiteralNode* node)
{
	_stream << "'";
	for (std::string::const_iterator str = node->literal().begin();
			str != node->literal().end(); ++str)
	{
		switch (*str)
		{
		case '\n':
			_stream << "\\n";
			break;
		case '\r':
			_stream << "\\r";
			break;
		case '\t':
			_stream << "\\t";
			break;
		case '\\':
			_stream << "\\\\";
			break;
		default:
			_stream << *str;
			break;
		}
	}
	_stream << "'";
}

void AstToCodePrinter::visitDoubleLiteralNode(DoubleLiteralNode* node)
{
	_stream << node->literal();
}

void AstToCodePrinter::visitIntLiteralNode(IntLiteralNode* node)
{
	_stream << node->literal();
}

void AstToCodePrinter::visitLoadNode(LoadNode* node)
{
	_stream << node->var()->name();
}

void AstToCodePrinter::visitStoreNode(StoreNode* node)
{
	_stream << node->var()->name() << " " << tokenOp(node->op()) << " ";
	node->value()->visit(this);
	// _stream << ";\n";
}

void AstToCodePrinter::visitForNode(ForNode* node)
{
	_stream << "for (" << node->var()->name() << " in ";
	node->inExpr()->visit(this);
	_stream << ")";
	node->body()->visit(this);
}

void AstToCodePrinter::visitWhileNode(WhileNode* node)
{
	_stream << "while (";
	node->whileExpr()->visit(this);
	_stream << ")";
	node->loopBlock()->visit(this);
}

void AstToCodePrinter::visitIfNode(IfNode* node)
{
	_stream << "if (";
	node->ifExpr()->visit(this);
	_stream << ")";
	node->thenBlock()->visit(this);
	if (node->elseBlock())
	{
		_stream << " else";
		node->elseBlock()->visit(this);
	}
}

void AstToCodePrinter::visitBlockNode(BlockNode* node)
{
	_stream << "{\n";
	processBlockNode(node);
	_stream << "}\n";
}

void AstToCodePrinter::visitFunctionNode(FunctionNode* node)
{
	_stream << "function " << typeToName(node->returnType()) << " "
			<< node->name() << "(";
	for (uint32_t i = 0; i < node->parametersNumber(); ++i)
	{
		if (i > 0)
			_stream << ", ";
		_stream << typeToName(node->parameterType(i)) << " "
				<< node->parameterName(i);
	}
	_stream << ")";

	if (node->body()->nodeAt(0)->isNativeCallNode())
	{
		node->body()->nodeAt(0)->visit(this);
	}
	else
	{
		node->body()->visit(this);
	}
}

void AstToCodePrinter::visitReturnNode(ReturnNode* node)
{
	_stream << "return ";
	if (node->returnExpr() != NULL)
	{
		node->returnExpr()->visit(this);
	}
	// _stream << ";\n";
}

void AstToCodePrinter::visitCallNode(CallNode* node)
{
	_stream << node->name() << "(";
	for (uint32_t i = 0; i != node->parametersNumber(); ++i)
	{
		if (i > 0)
			_stream << ", ";
		node->parameterAt(i)->visit(this);
	}
	_stream << ")";
}

void AstToCodePrinter::visitNativeCallNode(NativeCallNode* node)
{
	_stream << " native '" << node->nativeName() << "'";
}

void AstToCodePrinter::visitPrintNode(PrintNode* node)
{
	_stream << "print (";
	for (uint32_t i = 0; i != node->operands(); ++i)
	{
		if (i > 0)
			_stream << ", ";
		node->operandAt(i)->visit(this);
	}
	_stream << ")";
	//_stream << ");\n";
}

} /* namespace mathvm */
