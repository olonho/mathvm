#include <cstdlib>
#include <iostream>
#include <fstream>
#include <iterator>

#include "parser.h"

using namespace mathvm;

class SrcPrinterVisitor : public AstVisitor
{
public:
	SrcPrinterVisitor(std::ostream& out_stream): m_out_stream(out_stream), m_indent(0){ }

	void process(AstFunction* top)
	{
		m_indent = -1;
		top->node()->body()->visit(this);
	}

	virtual void visitBinaryOpNode(BinaryOpNode* node)
	{
		m_out_stream << "(";
		node->left()->visit(this);
		m_out_stream << tokenOp(node->kind());
		node->right()->visit(this);
		m_out_stream << ")";
	}

	virtual void visitUnaryOpNode(UnaryOpNode* node)
	{
		m_out_stream << tokenOp(node->kind());
		node->operand()->visit(this);
	}

	virtual void visitStringLiteralNode(StringLiteralNode* node)
	{
		m_out_stream << "'";
		for (std::size_t i = 0; i < node->literal().size(); ++i)
			switch(node->literal()[i])
			{
				case '\n': m_out_stream << "\\n";  break;
				case '\t': m_out_stream << "\\t";  break;
				case '\r': m_out_stream << "\\r";  break;
				case '\\': m_out_stream << "\\\\"; break;
				case '\'': m_out_stream << "\\'";  break;
				default: m_out_stream << node->literal()[i] ; break;
			}
		m_out_stream << "'";
	}

	virtual void visitDoubleLiteralNode(DoubleLiteralNode* node)
	{
		m_out_stream << node->literal();
	}

	virtual void visitIntLiteralNode(IntLiteralNode* node)
	{
		m_out_stream << node->literal();
	}

	virtual void visitLoadNode(LoadNode* node)
	{
		m_out_stream << node->var()->name();
	}

	virtual void visitStoreNode(StoreNode* node)
	{
		m_out_stream << node->var()->name() << tokenOp(node->op());
		node->value()->visit(this);
	}

	virtual void visitForNode(ForNode* node)
	{
		m_out_stream << "for(" << node->var()->name() << " in ";
		node->inExpr()->visit(this);
		m_out_stream << ")" << std::endl << "{" << std::endl;
		node->body()->visit(this);
		m_out_stream << "}";
	}

	virtual void visitWhileNode(WhileNode* node)
	{
		m_out_stream << "while (";
		node->whileExpr()->visit(this);
		m_out_stream <<") {" << std::endl;
		node->loopBlock()->visit(this);
		m_out_stream << "}";
	}

	virtual void visitIfNode(IfNode* node)
	{
		m_out_stream << "if (";
		node->ifExpr()->visit(this);
		m_out_stream << ") {" << std::endl;
		node->thenBlock()->visit(this);
		if (node->elseBlock())
		{
			m_out_stream << "} else {" << std::endl;
			node->elseBlock()->visit(this);
		}
		m_out_stream << "}";
	}

	virtual void visitBlockNode(BlockNode* node)
	{
		Scope::VarIterator it_v = node->scope();
		while (it_v.hasNext())
		{
			AstVar* var = it_v.next();
			m_out_stream << typeToName(var->type()) << " " << var->name() << ";" << std::endl;
		}
		Scope::FunctionIterator it_f = node->scope();
		while (it_f.hasNext())
			printFunction(it_f.next());
		for (std::size_t i = 0; i < node->nodes(); ++i)
		{
			node->nodeAt(i)->visit(this);
			if (!node->nodeAt(i)->isForNode() && !node->nodeAt(i)->isWhileNode() &&
				!node->nodeAt(i)->isIfNode())
				m_out_stream << ";";
			m_out_stream << std::endl;
		}
	}

	virtual void visitFunctionNode(FunctionNode* node)
	{
		m_out_stream << "function " << typeToName(node->returnType()) << " " << node->name() << "(";
		if (node->parametersNumber() != 0)
		{
			m_out_stream << typeToName(node->parameterType(0)) << " " << node->parameterName(0);
			for (std::size_t i = 1; i < node-> parametersNumber(); ++i)
				m_out_stream << ", " << typeToName(node->parameterType(i)) << " "
						     << node->parameterName(i);
		}
		m_out_stream << ")";
	}

	virtual void visitReturnNode(ReturnNode* node)
	{
		m_out_stream << "return";
		if (node->returnExpr())
		{
			m_out_stream << " ";
			node->returnExpr()->visit(this);
		}
	}

	virtual void visitCallNode(CallNode* node)
	{
		m_out_stream << node->name() << "(";
		if (node->parametersNumber() != 0)
		{
			node->parameterAt(0)->visit(this);
			for (std::size_t i = 1; i < node->parametersNumber(); ++i)
			{
				m_out_stream << ", ";
				node->parameterAt(i)->visit(this);
			}
		}
		m_out_stream << ")";
	}

	virtual void visitNativeCallNode(NativeCallNode* node)
	{
		m_out_stream << "native '" << node->nativeName() << "'";
	}

	virtual void visitPrintNode(PrintNode* node)
	{
		m_out_stream << "print(";
		if (node->operands() != 0)
		{
			node->operandAt(0)->visit(this);
			for (std::size_t i = 1; i < node->operands(); ++i)
			{
				m_out_stream << ", ";
				node->operandAt(i)->visit(this);
			}
		}
		m_out_stream << ")";
	}

private:
	void printFunction(AstFunction* func)
	{
		func->node()->visit(this);
		if (func->node()->body()->nodeAt(0)->isNativeCallNode())
		{
			m_out_stream << " ";
			func->node()->body()->nodeAt(0)->visit(this);
			m_out_stream << ";" << std::endl;
		}
		else
		{
			m_out_stream << "{" << std::endl;
			func->node()->body()->visit(this);
			m_out_stream << "}" << std::endl;
		}
	}

	std::ostream& m_out_stream;
	int           m_indent;
};

int main(int argc, char* argv[])
{
	if (argc != 2)
	{
		std::cout << "Usage: " << argv[0] << " <input source>" << std::endl;
		exit(1);
	}

	std::string   source_code;
	std::ifstream input(argv[1]);
	input.unsetf(std::ios_base::skipws);
	std::copy(std::istream_iterator<char>(input), std::istream_iterator<char>(), std::back_inserter(source_code));

//	std::cout << source_code << std::endl;

	Parser parser;
	if (Status* s = parser.parseProgram(source_code))
	{
		std::cerr << "PARSE ERROR: " << s->getError() << std::endl;
		exit(1);
	}

	AstFunction* top = parser.top();
	SrcPrinterVisitor src_printer(std::cout);
	src_printer.process(top);
	return 0;
}
