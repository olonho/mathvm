#include "mathvm.h"
#include "parser.h"
#include "visitors.h"
#include <iostream>

using namespace mathvm;
using std::ostream;
using std::cout;

class Printer: public AstVisitor
{
  public:
	Printer(AstFunction * top_m, ostream & result_m): result(result_m), top(top_m),  level(-1)
	{}

	void visitUnaryOpNode(UnaryOpNode * node)
	{
		result << tokenOp(node->kind());
		node->operand()->visit(this);
	}

	void visitBinaryOpNode(BinaryOpNode * node)
	{
		result << "(";
		node->left()->visit(this);
		result << tokenOp(node->kind());
		node->right()->visit(this);
		result << ")";
	}

	void visitBlockNode(BlockNode * node)
	{
		result << "\n";
		Scope::VarIterator it(node->scope());

		while(it.hasNext())
		{
			AstVar * var = it.next();
			result << std::string(level, ' ') << typeToName(var->type()) << " " << var->name() << ";\n";
		}

		Scope::FunctionIterator itf(node->scope());

		while(itf.hasNext())
		{
			result << std::string(level, ' ') << "\n";
			AstFunction * var = itf.next();
			var->node()->visit(this);
			result << std::string(level, ' ' ) << "\n";
		}

		for (uint32_t i = 0; i < node->nodes(); i++)
		{
			if (node->nodeAt(i)->isCallNode())
			{
				result << std::string(level, ' ');
				node->nodeAt(i)->visit(this);
				result << ";";
			}
			else
				node->nodeAt(i)->visit(this);

            result << "\n";
		}
	}

	void visitCallNode(CallNode * node)
	{
		result << node->name() << "(";

		if (node->parametersNumber() >= 1)
			node->parameterAt(0)->visit(this);

		for(uint32_t i = 1; i < node->parametersNumber(); i++)
		{
			result << ", ";
			node->parameterAt(i)->visit(this);
		}

		result << ")";
	}

	void visitDoubleLiteralNode(DoubleLiteralNode * node)
	{
		result << node->literal();
	}

	void visitForNode(ForNode * node)
	{
		result << std::string(level, ' ') << "for (" << node->var()->name() << " in ";
		++level;
		node->inExpr()->visit(this);
		result << " ) {";
		node->body()->visit(this);
		--level;
		result << std::string(level, ' ') << "}";
	}

	//native
	void visitFunctionNode(FunctionNode * node)
	{
		if (level == -1)
		{
			level += 1;
			node->body()->visit(this);
			--level;
		}
		else
		{
			result << std::string(level, ' ') << "function " << typeToName(node->returnType()) << " " << node->name() << "(";
			++level;

			if (node->parametersNumber() >= 1)
			{
				result << typeToName(node->parameterType(0)) << " " << node->parameterName(0);

				for(uint32_t i = 1; i < node->parametersNumber(); i++)
				{
					result << ", " << typeToName(node->parameterType(i)) << " " << node->parameterName(i);
				}
			}

			result << ")";
			--level;

			//native
			if (node->body()->nodeAt(0)->isNativeCallNode())
				node->body()->nodeAt(0)->visit(this);
			else
			{
				result << "{";
				node->body()->visit(this);
				result << std::string(level, ' ') << "}";
			}
		}
	}

	void visitNativeCallNode(NativeCallNode * node)
	{
		result << " native" << " '" << node->nativeName() << "';";
	}

	void visitIfNode(IfNode * node)
	{
		result << std::string(level, ' ') << "if (";
		node->ifExpr()->visit(this);
		result << ") { " ;
		++level;
		node->thenBlock()->visit(this);
		--level;
		result << std::string(level, ' ') << "} ";

		if(node->elseBlock() != NULL)
		{
			result << "else {";
			++level;
			result << std::string(level, ' ') ;
			node->elseBlock()->visit(this);
			--level;
			result << std::string(level, ' ') << "} ";
		}
	}
	void visitIntLiteralNode(IntLiteralNode * node)
	{
		result << node->literal();
	}
	void visitLoadNode(LoadNode * node)
	{
		result << node->var()->name();
	}

	void visitPrintNode(PrintNode * node)
	{
		result << std::string(level, ' ') << "print (";

		if (node->operands() >= 1)
			node->operandAt(0)->visit(this);

		for(int i = 1; i != node->operands(); ++i)
		{
			result << ", ";
			node ->operandAt(i)->visit(this);
		}

		result << ");";
	}

	void visitReturnNode(ReturnNode * node)
	{
        result << std::string(level, ' ') << "return ";
        if (node->returnExpr() != NULL)
        {
            node->returnExpr()->visit(this);
        }
        result << ";";
	}
	void visitStoreNode(StoreNode * node)
	{
		result << std::string(level, ' ') << node->var() ->name() << " " << tokenOp(node->op()) << " ";
		node->value()->visit(this);
		result << ";";
	}
	void visitStringLiteralNode(StringLiteralNode * node)
	{
		result << "'";
		string t = node->literal();

		if (node->literal().find("\n") != std::string::npos)
			t.replace(node->literal().find("\n"), 1, "\\n");

		if (node->literal().find("\r") != std::string::npos)
			t.replace(node->literal().find("\r"), 1, "\\r");

		result << t;
		result << "'";
	}

	void visitWhileNode(WhileNode * node)
	{
		result << std::string(level, ' ') << "while (";
		node ->whileExpr()->visit(this);
		result << ") {";
		++level;
		node->loopBlock()->visit(this);
		--level;
		result << std::string(level, ' ') << "}\n";
	}

	void print()
	{
		top->node()->visit(this);
	}

  private:
	ostream & result;
	AstFunction * top;
	size_t level;
};

class AstPrinter : public Translator
{
  public:
	virtual Status * translate(const string & program, Code ** code)
	{
		Parser parser;
		Status * status = parser.parseProgram(program);

		if (status && status->isError()) return status;

		Printer printer(parser.top(), cout);
		printer.print();
        return Status::Ok();
	}
};


Translator * Translator::create(const string & impl)
{
	if (impl == "printer")
	{
		return new AstPrinter();
	}
	else
	{
		return NULL;
	}
}
