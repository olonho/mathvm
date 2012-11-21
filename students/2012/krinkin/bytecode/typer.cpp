#include "typer.h"

void Typer::visitBinaryOpNode(BinaryOpNode *node)
{
    node->left()->visit(this);
    node->right()->visit(this);
    
    if (!check_number(node->left()) || !check_number(node->right())) return;

	switch (node->kind())
	{
	case tDECRSET:
	case tASSIGN:
	case tINCRSET:
	    type(node, type(node->left()));
		break;
	case tOR: case tAND:
	case tEQ: case tNEQ:
	case tGT: case tGE:
	case tLT: case tLE:
        type(node, VT_INT);
		break;
	case tMUL: case tADD:
	case tDIV: case tSUB:
        if (type(node->left()) == VT_DOUBLE || type(node->right()) == VT_DOUBLE)
            type(node, VT_DOUBLE);
        else
            type(node, VT_INT);
		break;
	case tMOD:
	    if (type(node->left()) == VT_DOUBLE || type(node->right()) == VT_DOUBLE)
	    {
	        error("modular ariphmetic is not allowed for DOUBLE", node);
	        return;
        }
        type(node, VT_INT);
		break;
	case tRANGE:
        if (type(node->left()) == VT_DOUBLE || type(node->right()) == VT_DOUBLE)
        {
            error("DOUBLE is not allowed here", node);
            return;
        }
        type(node, VT_VOID);
		break;
	default: assert(0);
	}
}

void Typer::visitUnaryOpNode(UnaryOpNode *node)
{
    node->operand()->visit(this);
    if (!check_number(node->operand())) return;
    
    switch (node->kind())
    {
    case tNOT:
        type(node, VT_INT);
        break;
    case tSUB:
    case tADD:
        type(node, type(node->operand()));
        break;
    default: assert(0);
    }
}

void Typer::visitStringLiteralNode(StringLiteralNode *node)
{
    type(node, VT_STRING);
}

void Typer::visitDoubleLiteralNode(DoubleLiteralNode *node)
{
    type(node, VT_DOUBLE);
}

void Typer::visitIntLiteralNode(IntLiteralNode *node)
{
    type(node, VT_INT);
}

void Typer::visitLoadNode(LoadNode *node)
{
    type(node, node->var()->type());
}

void Typer::visitStoreNode(StoreNode *node)
{
    node->value()->visit(this);
    if (!check_value(node->value())) type(node, VT_INVALID);
    if (node->var()->type() == VT_STRING || type(node->value()) == VT_STRING)
    {
        if (node->var()->type() != type(node->value()))
        {
            error("types mismatch", node);
            return;
        }
    }
    type(node, VT_VOID);
}

void Typer::visitForNode(ForNode *node)
{	
	if (!node->inExpr()->isBinaryOpNode())
	{
	    error("Binary operation expected", node->inExpr());
	    return;
	}
	BinaryOpNode *cond = node->inExpr()->asBinaryOpNode();
	if (cond->kind() != tRANGE)
	{
	    error("RANGE operator expected", cond);
	    return;
	}
    cond->visit(this);
	node->body()->visit(this);
}

void Typer::visitWhileNode(WhileNode *node)
{
	node->whileExpr()->visit(this);
	if (!check_number(node->whileExpr())) return;
	node->loopBlock()->visit(this);
}

void Typer::visitIfNode(IfNode *node)
{
	node->ifExpr()->visit(this);
	if (!check_number(node->ifExpr())) return;
	node->thenBlock()->visit(this);
	if (node->elseBlock() && node->elseBlock()->nodes())
	    node->elseBlock()->visit(this);
}

void Typer::visitBlockNode(BlockNode *node)
{
    push_scope(node->scope());
    check_scope();
    for (uint32_t i = 0; i != node->nodes(); ++i)
        node->nodeAt(i)->visit(this);
    pop_scope();
}

void Typer::visitFunctionNode(FunctionNode *node)
{
    push_function(node);
	if (node->body()->nodes() && !node->body()->nodeAt(0)->isNativeCallNode())
		node->body()->visit(this);
    pop_function();
}

void Typer::visitReturnNode(ReturnNode *node)
{
    FunctionNode *outer = function();
    if (node->returnExpr())
    {
        node->returnExpr()->visit(this);
        if (!check_value(node->returnExpr()))
        {
		    type(node, VT_INVALID);
		    return;
        }
    }
    type(node, outer->returnType());
}

void Typer::visitCallNode(CallNode *node)
{
    AstFunction *called = scope()->lookupFunction(node->name());
    if (called)
    {
        if (called->parametersNumber() != node->parametersNumber())
        {
            error("arguments number mismatch", node);
        }
        else
        {
			for (uint32_t i = 0; i != node->parametersNumber(); ++i)
			{
				node->parameterAt(i)->visit(this);
				if (!check_value(node->parameterAt(i)))
				{
				    type(node, VT_INVALID);
				    return;
				}
				if (type(node->parameterAt(i)) == VT_STRING || called->parameterType(i) == VT_STRING)
				{
				    if (type(node->parameterAt(i)) != called->parameterType(i))
				    {
				        error("arguments types mismatch", node);
				        type(node, VT_INVALID);
				        return;
				    }
				}
			}
			type(node, called->returnType());
        }
    }
    else
    {
        error("undefined function call", node);
    }
}

void Typer::visitNativeCallNode(NativeCallNode *node)
{
    type(node, function()->returnType());
}

void Typer::visitPrintNode(PrintNode *node)
{
    for (uint32_t i = 0; i != node->operands(); ++i)
	{
		node->operandAt(i)->visit(this);
		if (!check_value(node->operandAt(i))) return;
	}
}

Status *Typer::check(AstFunction *top)
{
    m_status = 0;
    m_mapping = new std::map<AstNode *, VarType>();
    
    push_function(top->node());
    push_scope(top->owner());

    top->node()->visit(this);

    pop_scope();
    pop_function();
    return m_status;
}

void Typer::check_scope()
{
	Scope::FunctionIterator ifun(scope());
	while (ifun.hasNext())
	{
		FunctionNode *fun = ifun.next()->node();
		fun->visit(this);
	}
}
