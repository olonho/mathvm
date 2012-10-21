#include "typechecker.h"

std::auto_ptr<Status> TypeChecker::check(AstFunction *top)
{
	m_status.reset();
	m_top_function = top->node();
	m_top_scope = top->owner();
	top->node()->visit(this);
	return m_status;
}

void TypeChecker::visitBinaryOpNode(BinaryOpNode *node)
{
	node->left()->visit(this);
	node->right()->visit(this);
	
	VarType left_type = get_type(node->left());
	VarType right_type = get_type(node->right());
	VarType common_type = common(left_type, right_type);
	VarType this_type = VT_INVALID;
	TokenKind op = node->kind();
	
	std::string const forbidden_type = std::string("forbidden expression type: ")
											+ typeToName(left_type) + " " + tokenOp(op)
											+ " " + typeToName(right_type);

	if ((left_type == VT_VOID) || (right_type == VT_VOID))
		declare_error(forbidden_type, node);

	switch (op)
	{
	case tDECRSET:
		if (left_type == VT_STRING)
			declare_error(forbidden_type, node);
	case tASSIGN: case tINCRSET:
		if (!lvalue(node->left()))
			declare_error("lvalue expected as left operand", node);
		else if (!convertable(left_type, right_type))
			declare_error(std::string("cannot convert ") + typeToName(left_type)
						+ " to " + typeToName(right_type), node);
		else
			this_type = left_type;
		break;
	case tOR: case tAND:
		if (!number(left_type) || !number(right_type))
			declare_error(forbidden_type, node);
		else
			this_type = VT_INT;
		break;
	case tEQ: case tNEQ:
	case tGT: case tGE:
	case tLT: case tLE:
		if (!convertable(left_type, right_type) && !convertable(right_type, left_type))
			declare_error(forbidden_type, node);
		else
			this_type = VT_INT;
		break;
	case tMUL:
		if ((left_type == VT_STRING) && (right_type == VT_INT))
			this_type = VT_STRING;
		else if (!number(left_type) || !number(right_type))
			declare_error(forbidden_type, node);
		else
			this_type = common_type;
		break;
	case tMOD:
		if ((left_type != VT_INT) || (right_type != VT_INT))
			declare_error(forbidden_type, node);
		else
			this_type = VT_INT;
		break;
	case tADD:
		if (!convertable(left_type, right_type) && !convertable(right_type, left_type))
			declare_error(forbidden_type, node);
		else
			this_type = common_type;
		break;
	case tSUB: case tDIV:
		if (!number(left_type) || !number(right_type))
			declare_error(forbidden_type, node);
		else
			this_type = common_type;
		break;
	case tRANGE:
		if (!number(left_type) || !number(right_type))
			declare_error(forbidden_type, node);
		else
			this_type = common_type;
		break;
	default:
		declare_error(forbidden_type, node);
		break;
	}
	set_type(node, this_type);
}

void TypeChecker::visitUnaryOpNode(UnaryOpNode *node)
{
	node->operand()->visit(this);
	VarType operand_type = get_type(node->operand());
	VarType this_type = VT_INVALID;
	TokenKind op = node->kind();
	
	std::string const forbidden_type = std::string("forbidden expression type: ") + tokenOp(op)
								+ typeToName(operand_type);
	
	switch (op)
	{
	case tNOT:
		if (!number(operand_type))
			declare_error(forbidden_type, node);
		else
			this_type = VT_INT;
		break;
	case tADD: case tSUB:
		if (!number(operand_type))
			declare_error(forbidden_type, node);
		else
			this_type = operand_type;
		break;
	default:
		declare_error(forbidden_type, node);
		break;
	}
	set_type(node, this_type);
}

void TypeChecker::visitStringLiteralNode(StringLiteralNode *node)
{
	set_type(node, VT_STRING);
}

void TypeChecker::visitDoubleLiteralNode(DoubleLiteralNode *node)
{
	set_type(node, VT_DOUBLE);
}

void TypeChecker::visitIntLiteralNode(IntLiteralNode *node)
{
	set_type(node, VT_INT);
}

void TypeChecker::visitLoadNode(LoadNode *node)
{
	set_type(node, node->var()->type());
}

void TypeChecker::visitStoreNode(StoreNode *node)
{
	node->value()->visit(this);
	VarType value_type = get_type(node->value());
	VarType var_type = node->var()->type();
	if (!convertable(var_type, value_type))
	{
		declare_error(std::string("cannot convert ") + typeToName(value_type)
							+ " to " + typeToName(var_type), node);
		set_type(node, VT_INVALID);
	}
	else
	{
		set_type(node, var_type);
	}
}

void TypeChecker::visitForNode(ForNode *node)
{
	node->inExpr()->visit(this);
	node->body()->visit(this);
	
	VarType this_type = VT_INVALID;
	
	BinaryOpNode *expr = dynamic_cast<BinaryOpNode*>(node->inExpr());
	if (!expr) declare_error("binary expression expected", expr);
	if (expr->kind() != tRANGE) declare_error(tokenOp(tRANGE) + std::string(" expected"), expr);
	if (!convertable(node->var()->type(), get_type(expr)))
		declare_error(std::string("cannot convert ") + typeToName(get_type(expr))
						+ " to " + typeToName(node->var()->type()), expr);
	else if (get_type(node->body()) != VT_INVALID)
		this_type = VT_VOID;
	set_type(node, this_type);
}

void TypeChecker::visitWhileNode(WhileNode *node)
{
	node->whileExpr()->visit(this);
	node->loopBlock()->visit(this);
	if ((get_type(node->whileExpr()) != VT_INVALID) && (get_type(node->loopBlock()) != VT_INVALID))
		set_type(node, VT_INVALID);
	else
		set_type(node, VT_VOID);
}

void TypeChecker::visitIfNode(IfNode *node)
{
	node->ifExpr()->visit(this);
	node->thenBlock()->visit(this);
	
	VarType this_type = VT_VOID;
	
	if (node->elseBlock() && node->elseBlock()->nodes())
	{
		node->elseBlock()->visit(this);
		this_type = get_type(node->elseBlock());
	}
	if (!number(get_type(node->ifExpr())))
	{
		declare_error("condition must be number", node);
		this_type = VT_INVALID;
	}
	if ((get_type(node->ifExpr()) != VT_INVALID) && (get_type(node->thenBlock()) != VT_INVALID))
		set_type(node, this_type);
	else
		set_type(node, VT_INVALID);
}

void TypeChecker::visitBlockNode(BlockNode *node)
{
	Scope *old_scope = m_top_scope;
	m_top_scope = node->scope();
	
	VarType result_type = check_scope(node->scope());
	for (uint32_t i = 0; i != node->nodes(); ++i)
	{
		node->nodeAt(i)->visit(this);
		VarType node_type = get_type(node->nodeAt(i));
		if (node_type == VT_INVALID) result_type = VT_INVALID;
	}
	set_type(node, result_type);
	
	m_top_scope = old_scope;
}

void TypeChecker::visitFunctionNode(FunctionNode *node)
{
	FunctionNode *old_top = m_top_function;
	m_top_function = node;
	
	if (node->body()->nodes() && node->body()->nodeAt(0)->isNativeCallNode())
		set_type(node->body(), node->returnType());
	else
		node->body()->visit(this);
	if (get_type(node->body()) != VT_INVALID) set_type(node, node->returnType());
	else set_type(node, VT_INVALID);
	
	m_top_function = old_top;
}

void TypeChecker::visitReturnNode(ReturnNode *node)
{
	VarType function_type = m_top_function->returnType();
	VarType return_type = VT_VOID;
	
	if (node->returnExpr())
	{
		node->returnExpr()->visit(this);
		return_type = get_type(node->returnExpr());
	}
	
	if (!convertable(function_type, return_type))
	{
		declare_error(std::string("cannot convert ") + typeToName(return_type)
									+ " to " + typeToName(function_type), node);
		return_type = VT_INVALID;
	}
	else
	{
		return_type = function_type;
	}

	set_type(node, return_type);
}

void TypeChecker::visitCallNode(CallNode *node)
{
	AstFunction *function = m_top_scope->lookupFunction(node->name());
	VarType this_type = VT_INVALID;
	if (function)
	{
		this_type = function->returnType();
		if (function->parametersNumber() != node->parametersNumber())
		{
			declare_error("arguments number mismatch", node);
			this_type = VT_INVALID;
		}
		else
		{
			for (uint32_t i = 0; i != node->parametersNumber(); ++i)
			{
				node->parameterAt(i)->visit(this);
				if (!convertable(function->parameterType(i), get_type(node->parameterAt(i))))
					this_type = VT_INVALID;
			}
		}
	}
	else
	{
		declare_error(std::string("undefined function ") + node->name(), node);
	}
	set_type(node, this_type);
}

void TypeChecker::visitNativeCallNode(NativeCallNode *node)
{
	set_type(node, m_top_function->returnType());
}

void TypeChecker::visitPrintNode(PrintNode *node)
{
	VarType this_type = VT_VOID;
	
	for (uint32_t i = 0; i != node->operands(); ++i)
	{
		node->operandAt(i)->visit(this);
		if ((get_type(node->operandAt(i)) == VT_INVALID) || (get_type(node->operandAt(i)) == VT_VOID))
			this_type = VT_INVALID;
	}
	set_type(node, this_type);
}

void TypeChecker::declare_error(std::string const &str, AstNode const * const node)
{
	if (!m_status.get()) m_status.reset( new Status(str, node->position()) );
}

bool TypeChecker::lvalue(AstNode const * const node) const
{
	LoadNode const * const var = dynamic_cast<LoadNode const * const>(node);
	if (var) return true;
	return false;
}

bool TypeChecker::convertable(VarType dst, VarType src) const
{
	if ((dst == VT_INVALID) || (src == VT_INVALID)) return false;
	if (dst == src) return true;
	if (dst == VT_STRING) return true;
	if (src == VT_STRING) return false;
	return true;
}

bool TypeChecker::number(VarType type) const
{
	return (type == VT_INT) || (type == VT_DOUBLE);
}

VarType TypeChecker::check_scope(Scope *scope)
{
	VarType result_type = VT_VOID;
	
	Scope::FunctionIterator ifun(scope);
	while (ifun.hasNext())
	{
		FunctionNode *fun = ifun.next()->node();
		fun->visit(this);
		if (get_type(fun) == VT_INVALID) result_type = VT_INVALID;
	}
	return result_type;
}

VarType common(VarType t1, VarType t2)
{
	if ((t1 == VT_STRING) || (t2 == VT_STRING)) return VT_STRING;
	if ((t1 == VT_DOUBLE) || (t2 == VT_DOUBLE)) return VT_DOUBLE;
	return VT_INT;
}

VarType get_type(CustomDataHolder const * const node)
{
	return static_cast<VarType>(reinterpret_cast<int>(node->info()));
}

void set_type(CustomDataHolder * const node, VarType type)
{
	node->setInfo(reinterpret_cast<void *>(static_cast<int>(type)));
}
