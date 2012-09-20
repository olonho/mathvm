#include "pretty.h"

void PrettyPrinter::visitTopLevelBlock(AstFunction const * const top)
{
	printBlock(top->node()->body());
}

void PrettyPrinter::visitBinaryOpNode(BinaryOpNode *node)
{
	m_out << "(";
	node->left()->visit(this);
	m_out << " " << tokenOp(node->kind()) << " ";
	node->right()->visit(this);
	m_out << ")";
}

void PrettyPrinter::visitUnaryOpNode(UnaryOpNode *node)
{
	m_out << tokenOp(node->kind());
	node->operand()->visit(this);
}

void PrettyPrinter::visitStringLiteralNode(StringLiteralNode *node)
{
	m_out << "'" << escape(node->literal()) << "'";
}

void PrettyPrinter::visitDoubleLiteralNode(DoubleLiteralNode *node)
{
	m_out << node->literal();
}

void PrettyPrinter::visitIntLiteralNode(IntLiteralNode *node)
{
	m_out << node->literal();
}

void PrettyPrinter::visitLoadNode(LoadNode *node)
{
	m_out << node->var()->name();
}

void PrettyPrinter::visitStoreNode(StoreNode *node)
{
	m_out << node->var()->name() << " " << tokenOp(node->op()) << " ";
	node->value()->visit(this);
}

void PrettyPrinter::visitForNode(ForNode *node)
{
	m_out << "for (" << node->var()->name() << " in " << node->inExpr()
	      << " )" << std::endl;
	node->body()->visit(this);		      
}

void PrettyPrinter::visitWhileNode(WhileNode *node)
{
	m_out << "while (";
	node->whileExpr()->visit(this);
	m_out << ")";
	// hmm... in ForLoop it is called body
	node->loopBlock()->visit(this);
}

void PrettyPrinter::visitIfNode(IfNode *node)
{
	m_out << "if (";
	node->ifExpr()->visit(this);
	m_out << ")" << std::endl;
	node->thenBlock()->visit(this);
	if (node->elseBlock() && node->elseBlock()->nodes())
	{
		m_out << "else" << std::endl;
		node->elseBlock()->visit(this);
	}
}

void PrettyPrinter::visitBlockNode(BlockNode *node)
{
	std::string indentation(m_indent, ' ');
	m_out << indentation << "{" << std::endl; m_indent += 4;
	printBlock(node);
	m_out << indentation << "}"; m_indent -= 4;
}

void PrettyPrinter::visitFunctionNode(FunctionNode *node)
{
	m_out << "function " << typeToName(node->returnType())
	      << " " << node->name() << "(";
	for (uint32_t i = 0; i != node->parametersNumber(); ++i)
	{
		m_out << typeToName(node->parameterType(i)) << " "
		      << node->parameterName(i);
		if (i != node->parametersNumber() - 1) m_out << ", ";
	}
	m_out << ") ";
	if (node->body()->nodes() && node->body()->nodeAt(0)->isNativeCallNode())
		node->body()->nodeAt(0)->visit(this);
	else
		node->body()->visit(this);
	m_out << std::endl;
}

void PrettyPrinter::visitReturnNode(ReturnNode *node)
{
	m_out << "return";
	if (node->returnExpr())
	{
		m_out << " ";
		node->returnExpr()->visit(this);
	}
}

void PrettyPrinter::visitCallNode(CallNode *node)
{
	m_out << node->name() << "(";
	for (uint32_t i = 0; i != node->parametersNumber(); ++i)
	{
		node->parameterAt(i)->visit(this);
		if (i != node->parametersNumber() - 1) m_out << ", ";
	}
	m_out << ")";
}

void PrettyPrinter::visitNativeCallNode(NativeCallNode *node)
{
	m_out << "native '" << node->nativeName() << "';";
}

void PrettyPrinter::visitPrintNode(PrintNode *node)
{
	m_out << "print(";
	for (uint32_t i = 0; i != node->operands(); ++i)
	{
		node->operandAt(i)->visit(this);
		if (i != node->operands() - 1) m_out << ", ";
	}
	m_out << ")";
}

void PrettyPrinter::printBlock(BlockNode *node)
{
	printScope(node->scope());
	std::string indentation(m_indent, ' ');
	for (uint32_t i = 0; i != node->nodes(); ++i)
	{
		m_out << indentation;
		node->nodeAt(i)->visit(this);
		m_out << ";" << std::endl;
	}
}

void PrettyPrinter::printScope(Scope *scope)
{
	std::string indentation(m_indent, ' ');
	//why constructor doesn't get const pointer?
	Scope::VarIterator ivar(scope);
	//Java-style iterators in C++?
	while (ivar.hasNext())
	{
		AstVar *var = ivar.next();
		m_out << indentation << typeToName(var->type()) << " "
		      << var->name() << ";" << std::endl;
	}
	
	Scope::FunctionIterator ifun(scope);
	while (ifun.hasNext()) ifun.next()->node()->visit(this);
}

std::string PrettyPrinter::escape(std::string const & str) const
{
	std::string result;
	for (auto i = std::begin(str); i != std::end(str); ++i)
	{
		switch (*i)
		{
		case '\n': result += "\\n"; break;
		case '\r': result += "\\r"; break;
		case '\\': result += "\\\\"; break;
		case '\t': result += "\\t"; break;
		default: result += *i; break;
		}
	}
	return result;
}
