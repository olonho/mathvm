#pragma once

#include <ast.h>

#include <ostream>
using std::ostream;
#include <string>
using std::string;
#include <stack>
using std::stack;
#include <vector>
using std::vector;
#include <memory>

#include "MyInstruction.h"
#include "StringStorage.h"
#include "MyVarScope.h"
#include "MyTypeStack.h"
#include "MyFuncScope.h"
#include "MyBytecode.h"
#include "MyProgram.h"

#include <iosfwd>

#define __int64 int64_t

struct nullstream {};

// Swallow all types
template <typename T>
inline nullstream & operator<<(nullstream & s, T const &) {return s;}

// Swallow manipulator templates
inline nullstream & operator<<(nullstream & s, std::ostream &(std::ostream&)) {return s;}

using namespace mathvm;

struct IndentHelper{
	IndentHelper(std::string& _indent) : indent(_indent){ indent.append("  "); }
	~IndentHelper(){ indent.resize(indent.length() - 2); }
	std::string& indent;
};


class PrintVisitor : public AstVisitor
{
public:
	PrintVisitor(std::ostream& out_stream, MyProgram* program = 0): m_out_stream(out_stream), lastFunction(0), curFuncID(-1), myProgram(program){}

	void process(AstFunction* top)
	{
		top->node()->body()->visit(this);
		myProgram->setStrings(strStorage.strings());
		// add fake return to end
		processInstruction(new Return());		
	}

	virtual void visitBinaryOpNode(BinaryOpNode* node)
	{
		IndentHelper h(indent);
		node->left()->visit(this);
		node->right()->visit(this);
		switch(node->kind())
		{
            	case tOR:       // ||
            	case tAND:      // &&
            	case tNEQ:      // !=
            	case tEQ:       // ==
		case tAOR:      // |
            	case tAAND:     // &
		case tAXOR:     // ^
            	case tMOD:      // %
				{
					if (myTypeStack.second() != VT_INT)
					{
						processInstruction(new Swap());
						processInstruction(MyInstruction::GetCast(myTypeStack.top(), VT_INT));
						processInstruction(new Swap());
					}
					if (myTypeStack.top() != VT_INT) 
						processInstruction(MyInstruction::GetCast(myTypeStack.top(), VT_INT));

					processInstruction(Integer::GetByKind(node->kind()));
				}
				break;
            case tGT:       // >
            case tGE:       // >=
            case tLT:       // <
            case tLE:       // <=
			case tADD:      // +
            case tSUB:      // -
            case tMUL:      // *
            case tDIV:      // /
				{
					VarType type = myTypeStack.makeTwoSameNumeric();
					if (myTypeStack.top() != type)
						processInstruction(MyInstruction::GetCast(myTypeStack.top(), type));

					processInstruction(Numeric::GetByKind(node->kind(), type));
				}
				break;
		default:
			throw std::logic_error("incorrect op");
		}
	}

	virtual void visitUnaryOpNode(UnaryOpNode* node)
	{
		IndentHelper h(indent);
		node->operand()->visit(this);
        switch (node->kind()) {
            case tNOT:  // "!"
				{
					if (myTypeStack.top() != VT_INT)
						processInstruction(MyInstruction::GetCast(myTypeStack.top(), VT_INT));

					processInstruction(new INOT());
				}
                return;
            case tSUB:  // "-"
				{				
					if (myTypeStack.top() == VT_INT)
						processInstruction(new NEG<__int64>());
					else if (myTypeStack.top() == VT_DOUBLE)
						processInstruction(new NEG<double>());
					else
						throw std::logic_error("bad stack");
				}
                return;
		default:
			throw std::logic_error("bad un op");
		}
	}

	virtual void visitStringLiteralNode(StringLiteralNode* node)
	{
		processInstruction(new ConstLoad<string>(node->literal(), &strStorage));
	}

	virtual void visitDoubleLiteralNode(DoubleLiteralNode* node)
	{
		processInstruction(new ConstLoad<double>(node->literal()));
	}

	virtual void visitIntLiteralNode(IntLiteralNode* node)
	{
		processInstruction(new ConstLoad<__int64>(node->literal()));
	}

	virtual void visitLoadNode(LoadNode* node)
	{
		processInstruction(new Load(node->var()->type(), node->var()->name(), &myVarScope));
	}

	virtual void visitStoreNode(StoreNode* node)
	{
		node->value()->visit(this);

		if (myTypeStack.top() != node->var()->type())
			processInstruction(MyInstruction::GetCast(myTypeStack.top(), node->var()->type()));

		// in 5:00 realization of incrset and decrset :(
		bool isDecOrInc = false;
		switch (node->op())
		{
		case tINCRSET:
		case tDECRSET:
			processInstruction(new Load(node->var()->type(), node->var()->name(), &myVarScope));
			isDecOrInc = true;
			break;
		default: break;
		}

		if (isDecOrInc && node->op() == tINCRSET)
		{
			if (node->var()->type() == VT_INT)    processInstruction(new ADD<__int64>());
			if (node->var()->type() == VT_DOUBLE) processInstruction(new ADD<double>());
		}

		if (isDecOrInc && node->op() == tDECRSET)
		{
			processInstruction(new Swap());
			if (node->var()->type() == VT_INT)    processInstruction(new SUB<__int64>());
			if (node->var()->type() == VT_DOUBLE) processInstruction(new SUB<double>());
		}

		processInstruction(MyInstruction::GetStore(node->var()->type(), node->var()->name(), &myVarScope));
	}

	virtual void visitForNode(ForNode* node)
	{
		throw std::runtime_error("not implemented yet");
		m_out_stream << indent << "ForNode" << std::endl;
		IndentHelper h(indent);
		node->inExpr()->visit(this);
		node->body()->visit(this);
	}

	virtual void visitWhileNode(WhileNode* node)
	{
		IndentHelper h(indent);
		MyLabel* labelBefore = new MyLabel();
		MyLabel* labelAfter = new MyLabel();

		processLabel(labelBefore);

		node->whileExpr()->visit(this);	
		processInstruction(new JNIF(labelAfter));

		node->loopBlock()->visit(this);
		processInstruction(new JA(labelBefore));

		processLabel(labelAfter);

		if (!myProgram)
		{
			delete labelAfter;
			delete labelBefore;
		}
	}

	virtual void visitIfNode(IfNode* node)
	{
		IndentHelper h(indent);

		MyLabel* labelElse = new MyLabel();
		MyLabel* labelAfter = new MyLabel();

		node->ifExpr()->visit(this);
		processInstruction(new JNIF(labelElse));

		node->thenBlock()->visit(this);
		processInstruction(new JA(labelAfter));

		processLabel(labelElse);
		if (node->elseBlock())
		{
			node->elseBlock()->visit(this);
		}
		processLabel(labelAfter);

		if (!myProgram)
		{
			delete labelAfter;
			delete labelElse;
		}
	}

	virtual void visitBlockNode(BlockNode* node)
	{
		IndentHelper h(indent);
		//m_out_stream << indent << "--- start context" << std::endl;
		myVarScope.incMem();
		myFuncScope.incMem();

		processInstruction(new INCM());

		FunctionNode* tmp = lastFunction;
		lastFunction = 0;
		if (tmp)
			tmp->visit(this);

		Scope::VarIterator it_v = node->scope();
		while (it_v.hasNext())
		{
			AstVar* v = it_v.next();
			myVarScope.getStoreID(v->type(), v->name());
		}

		Scope::FunctionIterator it_f = node->scope();
		while (it_f.hasNext())
		{
			AstFunction* f = (AstFunction*)it_f.next();
			myFuncScope.saveFuncID(f->node()->signature(), f->node()->name());
		}
		it_f = node->scope();
		while (it_f.hasNext())
		{
			AstFunction* f = (AstFunction*)it_f.next();
			int id = myFuncScope.getFuncID(f->node()->name()).second;
			int lastID = curFuncID;
			curFuncID = id;
			printFunction(f, id);
			curFuncID = lastID;
		}
		for (std::size_t i = 0; i < node->nodes(); ++i)
			node->nodeAt(i)->visit(this);

		// add fake return to main
		//if (curFuncID == -1)
		//	processInstruction(new Return());
		processInstruction(new DECM());

		myVarScope.decMem();
		myFuncScope.decMem();
		//m_out_stream << indent << "--- end context" << std::endl;
	}

	virtual void visitFunctionNode(FunctionNode* node)
	{
		IndentHelper h(indent);
		for (size_t i = 1; i < node->signature().size(); ++i)
		{
			//if (myTypeStack.top() != node->signature()[i].first)
			//	processInstruction(MyInstruction::GetCast(myTypeStack.top(), node->signature()[i].first), false);

			processInstruction(MyInstruction::GetStore(node->signature()[i].first, node->signature()[i].second, &myVarScope), false);
		}
	}

	virtual void visitReturnNode(ReturnNode* node)
	{
		IndentHelper h(indent);
		if (node->returnExpr())
		{
			node->returnExpr()->visit(this);
		}
		processInstruction(new Return());
	}

	virtual void visitCallNode(CallNode* node)
	{
		IndentHelper h(indent);
		if (node->parametersNumber() != 0)
		{
			for (int i = node->parametersNumber() - 1; i >= 0; --i)
				node->parameterAt(i)->visit(this);
		}
		processInstruction(new CALL(node->name(), &myFuncScope));
	}

	virtual void visitNativeCallNode(NativeCallNode* node)
	{
		throw std::logic_error("not implemented yet");
	}

	virtual void visitPrintNode(PrintNode* node)
	{
		IndentHelper h(indent);
		if (node->operands() != 0)
		{
			for (std::size_t i = 0; i < node->operands(); ++i)
			{
				node->operandAt(i)->visit(this);
				switch (myTypeStack.top())
				{
				case VT_INT:
					processInstruction(new PRINT<__int64>());
					break;
				case VT_DOUBLE:
					processInstruction(new PRINT<double>());
					break;
				case VT_STRING:
					processInstruction(new PRINT<string>());
					break;
				default:
					throw std::logic_error("bad stack");
				}
			}
		}
	}

private:
	void printFunction(AstFunction* func, int id)
	{
		IndentHelper h(indent);
		//cout << indent << "function " << id << " #" << func->name() << endl;
		lastFunction = func->node();

		if (func->node()->body()->nodeAt(0)->isNativeCallNode())
			func->node()->body()->nodeAt(0)->visit(this);
		else
			func->node()->body()->visit(this);
	}

	std::string indent;
	std::ostream& m_out_stream;

	FunctionNode* lastFunction;
	StringStorage strStorage;
	MyVarScope    myVarScope;
	MyTypeStack   myTypeStack;
	MyFuncScope   myFuncScope;

	int curFuncID;
	MyProgram* myProgram;

	void processInstruction(MyInstruction* i, bool processStack = true)
	{
		//m_out_stream << indent << i->text() << endl;
		if (processStack)
			i->processStack(&myTypeStack);

		if (myProgram)
			myProgram->addInstruction(curFuncID, i);
		else
			delete i;
	}

	void processLabel(MyLabel* label)
	{
		//m_out_stream << indent << label->text() << endl;
		if (myProgram)
			myProgram->addInstruction(curFuncID, label);
	}
};
