#include "asttranslator.h"

std::auto_ptr<Status> AstTranslator::translate(AstFunction *top, Code *code)
{
	m_status.reset();
	m_code = code;
	
	m_variables.clear();
	m_functions.clear();
	m_natives.clear();

	Signature convert_int_to_string_signature;
	convert_int_to_string_signature.push_back(SignatureElement(VT_STRING, "return"));
	convert_int_to_string_signature.push_back(SignatureElement(VT_INT, "value"));
	declare_native("$convert_int_to_string$", convert_int_to_string_signature);
	
	Signature convert_double_to_string_signature;
	convert_double_to_string_signature.push_back(SignatureElement(VT_STRING, "return"));
	convert_double_to_string_signature.push_back(SignatureElement(VT_DOUBLE, "value"));
	declare_native("$convert_double_to_string$", convert_double_to_string_signature);

	Signature strcmp_signature;
	strcmp_signature.push_back(SignatureElement(VT_INT, "return"));
	strcmp_signature.push_back(SignatureElement(VT_STRING, "str1"));
	strcmp_signature.push_back(SignatureElement(VT_STRING, "str2"));
	declare_native("$strcmp$", strcmp_signature);

	Signature concat_strings_signature;
	concat_strings_signature.push_back(SignatureElement(VT_STRING, "return"));
	concat_strings_signature.push_back(SignatureElement(VT_STRING, "str1"));
	concat_strings_signature.push_back(SignatureElement(VT_STRING, "str2"));
	declare_native("$concat_strings$", concat_strings_signature);

	Signature multiple_string_signature;
	multiple_string_signature.push_back(SignatureElement(VT_STRING, "return"));
	multiple_string_signature.push_back(SignatureElement(VT_STRING, "str"));
	multiple_string_signature.push_back(SignatureElement(VT_INT, "times"));
	declare_native("$multiple_string$", multiple_string_signature);

	declare_function(top);
	
	m_bytecode_stack = std::stack<BytecodeFunction *>();
	m_bytecode_stack.push(lookup_bytecode(top));

	m_scope_stack = std::stack<Scope *>();
	m_scope_stack.push(top->owner());

	top->node()->body()->visit(this);
	return m_status;
}

void AstTranslator::visitBinaryOpNode(BinaryOpNode *node)
{
	VarType target_type = get_type(node);	
	assert(target_type != VT_INVALID);
	switch (node->kind())
	{
	case tOR: case tAND:
	{
		do_logic(node);
		break;
	}
	case tMUL: case tDIV:
	case tADD: case tSUB:
	case tMOD:
	{
		if (target_type != VT_STRING)
		{
			node->right()->visit(this); convert_to(get_type(node->right()), target_type);
			node->left()->visit(this); convert_to(get_type(node->left()), target_type);
			do_binary_operation(node->kind(), target_type, target_type);
		}
		else
		{
			node->right()->visit(this);
			node->left()->visit(this);
			do_binary_operation(node->kind(), get_type(node->right()), get_type(node->left()));
		}
		break;
	}
	case tEQ: case tNEQ:
	case tGT: case tGE:
	case tLT: case tLE:
	{
		node->right()->visit(this); convert_to(get_type(node->right()), target_type);
		node->left()->visit(this); convert_to(get_type(node->left()), target_type);
		do_comparision(node->kind(), target_type);
		break;
	}
	case tDECRSET: case tINCRSET: case tASSIGN:
	{
		LoadNode const * const load = dynamic_cast<LoadNode *>(node->left());
		AstVar const * const var = load->var();
		node->right()->visit(this);
		do_assignment(var, node->kind(), get_type(node->right()));
		break;
	}
	default: assert(0);
	}
}

void AstTranslator::visitUnaryOpNode(UnaryOpNode *node)
{
	node->operand()->visit(this);
	do_unary_operation(node->kind(), get_type(node->operand()));
}

void AstTranslator::visitStringLiteralNode(StringLiteralNode *node)
{
	load_const(node->literal());
}

void AstTranslator::visitDoubleLiteralNode(DoubleLiteralNode *node)
{
	load_const(node->literal());
}

void AstTranslator::visitIntLiteralNode(IntLiteralNode *node)
{
	load_const(node->literal());
}

void AstTranslator::visitLoadNode(LoadNode *node)
{
	load_var(node->var());
}

void AstTranslator::visitStoreNode(StoreNode *node)
{
	node->value()->visit(this);
	save_var(node->var(), get_type(node->value()));
}

void AstTranslator::visitForNode(ForNode *node)
{
	BinaryOpNode *bin = dynamic_cast<BinaryOpNode *>(node->inExpr());
	bin->right()->visit(this);
	convert_to(get_type(bin->right()), get_type(bin));
	bin->left()->visit(this);
	convert_to(get_type(bin->left()), get_type(bin));
	uint32_t loop_jump_bci = bytecode()->current();
	dup(get_type(bin));
	save_var(node->var(), get_type(bin));
	bytecode()->addInsn(BC_IFICMPGE);
	uint32_t end_jump_bci = bytecode()->current();
	bytecode()->addInt16(0);
	node->body()->visit(this);
	switch (get_type(bin))
	{
	case VT_INT:
		load_const((int64_t)1);
		break;
	case VT_DOUBLE:
		load_const(1.0);
		break;
	default:
		assert(0);
	}
	do_binary_operation(tADD, get_type(bin), get_type(bin));
	bytecode()->addInsn(BC_JA);
	int16_t offset = loop_jump_bci - bytecode()->current();
	bytecode()->addInt16(offset);
	offset = bytecode()->current() - end_jump_bci;
	bytecode()->setInt16(end_jump_bci, offset);
	bytecode()->addInsn(BC_POP);
	bytecode()->addInsn(BC_POP);
}

void AstTranslator::visitWhileNode(WhileNode *node)
{	
	load_const((int64_t)0);
	uint32_t expr_label = bytecode()->current();
	node->whileExpr()->visit(this);
	convert_to_logic(get_type(node->whileExpr()));
	bytecode()->addInsn(BC_IFICMPE);
	uint32_t end_loop_jump = bytecode()->current();
	bytecode()->addInt16(0);
	bytecode()->addInsn(BC_POP);
	node->loopBlock()->visit(this);
	bytecode()->addInsn(BC_JA);
	int16_t offset = expr_label - bytecode()->current();
	bytecode()->addInt16(offset);
	offset = bytecode()->current() - end_loop_jump;
	bytecode()->setInt16(end_loop_jump, offset);
	bytecode()->addInsn(BC_POP);
}

void AstTranslator::visitIfNode(IfNode *node)
{
	int16_t offset;
	node->ifExpr()->visit(this);
	convert_to_logic(get_type(node->ifExpr()));
	load_const((int64_t)0);
	bytecode()->addInsn(BC_IFICMPE);
	uint32_t false_jump_bci = bytecode()->current();
	bytecode()->addInt16(0);
	node->thenBlock()->visit(this);
	if (node->elseBlock() && node->elseBlock()->nodes())
	{
		bytecode()->addInsn(BC_JA);
		uint32_t continue_jump_bci = bytecode()->current();
		bytecode()->addInt16(0);
		offset = bytecode()->current() - false_jump_bci;
		bytecode()->setInt16(false_jump_bci, offset);
		node->elseBlock()->visit(this);
		offset = bytecode()->current() - continue_jump_bci;
		bytecode()->setInt16(continue_jump_bci, offset);
	}
	else
	{
		offset = bytecode()->current() - false_jump_bci;
		bytecode()->setInt16(false_jump_bci, offset);
	}
	bytecode()->addInsn(BC_POP);
	bytecode()->addInsn(BC_POP);
}

void AstTranslator::visitBlockNode(BlockNode *node)
{
	m_scope_stack.push(node->scope());
	visit_scope(node->scope());
	for (uint32_t i = 0; i != node->nodes(); ++i)
		node->nodeAt(i)->visit(this);
	m_scope_stack.pop();
}

void AstTranslator::visitFunctionNode(FunctionNode *node)
{
	AstFunction *function = lookup_function(node->name());
	m_bytecode_stack.push(lookup_bytecode(function));
	if (node->body()->nodes() && node->body()->nodeAt(0)->isNativeCallNode())
		node->body()->nodeAt(0)->visit(this);
	else
	{
		m_scope_stack.push(node->body()->scope()->parent());
		visit_scope(node->body()->scope()->parent());
		for (uint32_t i = 0; i != node->parametersNumber(); ++i)
		{
			AstVar const * const local = lookup_variable(node->parameterName(node->parametersNumber() - i - 1));
			save_var(local, local->type());
		}
		node->body()->visit(this);
		m_scope_stack.pop();
	}
	m_bytecode_stack.pop();
}

void AstTranslator::visitReturnNode(ReturnNode *node)
{
	VarType return_type = get_type(node);
	if (return_type != VT_VOID)
	{
		node->returnExpr()->visit(this);
		convert_to(get_type(node->returnExpr()), return_type);
	}
	bytecode()->addInsn(BC_RETURN);
}

void AstTranslator::visitCallNode(CallNode *node)
{
	AstFunction *function = lookup_function(node->name());
	uint16_t id = lookup_function(function);
	for (uint32_t i = 0; i != node->parametersNumber(); ++i)
	{
		node->parameterAt(i)->visit(this);
		if (get_type(node->parameterAt(i)) != function->parameterType(i))
			convert_to(get_type(node->parameterAt(i)), function->parameterType(i));
	}
	bytecode()->addInsn(BC_CALL);
	bytecode()->addInt16(id);
}

void AstTranslator::visitNativeCallNode(NativeCallNode *node)
{
	uint16_t id = m_code->makeNativeFunction(node->nativeName(), node->nativeSignature(), 0);
	bytecode()->addInsn(BC_CALLNATIVE);
	bytecode()->addInt16(id);
	bytecode()->addInsn(BC_RETURN);
}

void AstTranslator::visitPrintNode(PrintNode *node)
{
	for (uint32_t i = 0; i != node->operands(); ++i)
	{
		node->operandAt(i)->visit(this);
		switch (get_type(node->operandAt(i)))
		{
		case VT_INT:
			bytecode()->addInsn(BC_IPRINT);
			break;
		case VT_DOUBLE:
			bytecode()->addInsn(BC_DPRINT);
			break;
		case VT_STRING:
			bytecode()->addInsn(BC_SPRINT);
			break;
		default: assert(0);
		}
	}
}

Bytecode *AstTranslator::bytecode() const
{
	return m_bytecode_stack.top()->bytecode();
}

uint16_t AstTranslator::locals() const
{
	return m_bytecode_stack.top()->localsNumber();
}

void AstTranslator::add_local(VarType type)
{
	m_bytecode_stack.top()->setLocalsNumber(locals() + size(type));
}

Scope *AstTranslator::scope() const
{
	return m_scope_stack.top();
}

uint16_t AstTranslator::id() const
{
	return m_bytecode_stack.top()->id();
}

void AstTranslator::convert_to_logic(VarType source_type)
{
	assert( (source_type == VT_INT) || (source_type == VT_DOUBLE) );
	
	if (source_type == VT_INT)
	{
		bytecode()->addInsn(BC_ILOAD0);
		bytecode()->addInsn(BC_ICMP);
	}
	else
	{
		bytecode()->addInsn(BC_DLOAD0);
		bytecode()->addInsn(BC_DCMP);
	}
}

void AstTranslator::convert_to(VarType source_type, VarType target_type)
{
	if (source_type != target_type)
	{
		if ( (source_type == VT_INT) && (target_type == VT_DOUBLE) )
			bytecode()->addInsn(BC_I2D);
		else if ( (source_type == VT_DOUBLE) && (target_type == VT_INT) )
			bytecode()->addInsn(BC_D2I);
		else if ( (source_type == VT_INT) && (target_type == VT_STRING) )
			call_native("$convert_int_to_string$");
		else if ( (source_type == VT_DOUBLE) && (target_type == VT_STRING) )
			call_native("$convert_double_to_string$");
	}
}

void AstTranslator::dup(VarType source_type)
{
	switch (source_type)
	{
	case VT_INT:
		bytecode()->addInsn(BC_STOREIVAR0);
		bytecode()->addInsn(BC_LOADIVAR0);
		bytecode()->addInsn(BC_LOADIVAR0);
		break;
	case VT_DOUBLE:
		bytecode()->addInsn(BC_STOREDVAR0);
		bytecode()->addInsn(BC_LOADDVAR0);
		bytecode()->addInsn(BC_LOADDVAR0);
		break;
	case VT_STRING:
		bytecode()->addInsn(BC_STORESVAR0);
		bytecode()->addInsn(BC_LOADSVAR0);
		bytecode()->addInsn(BC_LOADSVAR0);
		break;
	default: assert(0);
	}
}

void AstTranslator::save_var(AstVar const * const var, VarType source_type)
{
	std::pair<uint16_t, uint16_t> var_id = lookup_variable(var);
	
	convert_to(source_type, var->type());
	
	switch (var->type())
	{
	case VT_INT:
		if (var_id.first != id())
		{
			bytecode()->addInsn(BC_STORECTXIVAR);
			bytecode()->addInt16(var_id.first);
			bytecode()->addInt16(var_id.second);
		}
		else
		{
			bytecode()->addInsn(BC_STOREIVAR);
			bytecode()->addInt16(var_id.second);
		}
		break;
	case VT_DOUBLE:
		if (var_id.first != id())
		{
			bytecode()->addInsn(BC_STORECTXDVAR);
			bytecode()->addInt16(var_id.first);
			bytecode()->addInt16(var_id.second);
		}
		else
		{
			bytecode()->addInsn(BC_STOREDVAR);
			bytecode()->addInt16(var_id.second);
		}
		break;
	case VT_STRING:
		if (var_id.first != id())
		{
			bytecode()->addInsn(BC_STORECTXSVAR);
			bytecode()->addInt16(var_id.first);
			bytecode()->addInt16(var_id.second);
		}
		else
		{
			bytecode()->addInsn(BC_STORESVAR);
			bytecode()->addInt16(var_id.second);
		}
		break;
	default: assert(0);
	}
}

void AstTranslator::load_var(AstVar const * const var)
{
	std::pair<uint16_t, uint16_t> var_id = lookup_variable(var);
	
	switch (var->type())
	{
	case VT_INT:
		if (var_id.first != id())
		{
			bytecode()->addInsn(BC_LOADCTXIVAR);
			bytecode()->addInt16(var_id.first);
			bytecode()->addInt16(var_id.second);
		}
		else
		{
			bytecode()->addInsn(BC_LOADIVAR);
			bytecode()->addInt16(var_id.second);
		}
		break;
	case VT_DOUBLE:
		if (var_id.first != id())
		{
			bytecode()->addInsn(BC_LOADCTXDVAR);
			bytecode()->addInt16(var_id.first);
			bytecode()->addInt16(var_id.second);
		}
		else
		{
			bytecode()->addInsn(BC_LOADDVAR);
			bytecode()->addInt16(var_id.second);
		}
		break;
	case VT_STRING:
		if (var_id.first != id())
		{
			bytecode()->addInsn(BC_LOADCTXSVAR);
			bytecode()->addInt16(var_id.first);
			bytecode()->addInt16(var_id.second);
		}
		else
		{
			bytecode()->addInsn(BC_LOADSVAR);
			bytecode()->addInt16(var_id.second);
		}
		break;
	default: assert(0);
	}
}

void AstTranslator::load_const(int64_t value)
{
	if (value == 0) bytecode()->addInsn(BC_ILOAD0);
	else if (value == 1) bytecode()->addInsn(BC_ILOAD1);
	else if (value == -1) bytecode()->addInsn(BC_ILOADM1);
	else
	{
		bytecode()->addInsn(BC_ILOAD);
		bytecode()->addTyped(value);
	}
}

void AstTranslator::load_const(double value)
{
	if (value == 0.0) bytecode()->addInsn(BC_DLOAD0);
	else if (value == 1.0) bytecode()->addInsn(BC_DLOAD1);
	else if (value == -1.0) bytecode()->addInsn(BC_DLOADM1);
	else
	{
		bytecode()->addInsn(BC_DLOAD);
		bytecode()->addTyped(value);
	}
}

void AstTranslator::load_const(std::string const &value)
{
	if (value.empty()) bytecode()->addInsn(BC_SLOAD0);
	else
	{
		bytecode()->addInsn(BC_SLOAD);
		bytecode()->addInt16(m_code->makeStringConstant(value));
	}
}

void AstTranslator::call_native(std::string const &name)
{
	uint16_t id = lookup_native(name);
	bytecode()->addInsn(BC_CALLNATIVE);
	bytecode()->addInt16(id);
}

void AstTranslator::do_comparision(TokenKind kind, VarType source_type)
{
	switch (source_type)
	{
	case VT_INT:
		bytecode()->addInsn(BC_ICMP);
		break;
	case VT_DOUBLE:
		bytecode()->addInsn(BC_DCMP);
		break;
	case VT_STRING:
		call_native("$strcmp$");
		break;
	default: assert(0);
	}
	
	if (kind != tNEQ)
	{
		bytecode()->addInsn(BC_ILOAD0);
		bytecode()->addInsn(BC_SWAP);
		switch (kind)
		{
		case tGE:
			bytecode()->addInsn(BC_IFICMPGE);
			break;
		case tGT:
			bytecode()->addInsn(BC_IFICMPG);
			break;
		case tLT:
			bytecode()->addInsn(BC_IFICMPL);
			break;
		case tLE:
			bytecode()->addInsn(BC_IFICMPLE);
			break;
		case tEQ:
			bytecode()->addInsn(BC_IFICMPE);
			break;
		default: assert(0);
		}

		int16_t offset;
		uint32_t true_jump_offset = bytecode()->current();
		bytecode()->addInt16(0);
		bytecode()->addInsn(BC_POP);
		bytecode()->addInsn(BC_POP);
		bytecode()->addInsn(BC_ILOAD0);
		bytecode()->addInsn(BC_JA);
		uint32_t false_jump_offset = bytecode()->current();
		bytecode()->addInt16(0);
		offset = bytecode()->current() - true_jump_offset;
		bytecode()->setInt16(true_jump_offset, offset);
		bytecode()->addInsn(BC_POP);
		bytecode()->addInsn(BC_POP);
		bytecode()->addInsn(BC_ILOAD1);
		offset = bytecode()->current() - false_jump_offset;
		bytecode()->setInt16(false_jump_offset, offset);
	}
}

void AstTranslator::do_binary_operation(TokenKind kind, VarType upper_type, VarType lower_type)
{
	if ( (upper_type == VT_STRING) || (lower_type == VT_STRING) )
	{
		if (kind == tADD) call_native("$concat_strings");
		else if (upper_type == VT_STRING)
		{
			bytecode()->addInsn(BC_SWAP);
			call_native("$multiple_string$");
		}
	}
	else
	{
		switch (kind)
		{
		case tMUL:
			if (upper_type == VT_STRING) bytecode()->addInsn(BC_IMUL);
			else bytecode()->addInsn(BC_DMUL);
			break;
		case tDIV:
			if (upper_type == VT_STRING) bytecode()->addInsn(BC_IDIV);
			else bytecode()->addInsn(BC_DDIV);
			break;
		case tADD:
			if (upper_type == VT_STRING) bytecode()->addInsn(BC_IADD);
			else bytecode()->addInsn(BC_DADD);
			break;
		case tSUB:
			if (upper_type == VT_STRING) bytecode()->addInsn(BC_ISUB);
			else bytecode()->addInsn(BC_DSUB);
			break;
		case tMOD:
			if (upper_type == VT_STRING) bytecode()->addInsn(BC_IMOD);
			else assert(0);
			break;
		default: assert(0);
		}
	}
}

void AstTranslator::do_unary_operation(TokenKind kind, VarType source_type)
{
	if (kind != tADD)
	{
		if (kind == tSUB)
		{
			if (source_type == VT_INT) bytecode()->addInsn(BC_INEG);
			else bytecode()->addInsn(BC_DNEG);
		}
		else if (kind == tNOT)
		{
			convert_to_logic(source_type);
			load_const((int64_t)0);
			do_comparision(tEQ, VT_INT);
		}
	}
}

void AstTranslator::do_assignment(AstVar const * const var, TokenKind kind, VarType source_type)
{
	convert_to(source_type, var->type());
	
	if (kind == tINCRSET)
	{
		load_var(var);
		do_binary_operation(tADD, var->type(), var->type());
	}
	else if (kind == tDECRSET)
	{
		load_var(var);
		bytecode()->addInsn(BC_SWAP);
		do_binary_operation(tSUB, var->type(), var->type());
	}
	dup(var->type());
	save_var(var, var->type());
}

void AstTranslator::do_logic(BinaryOpNode const * const node)
{
	int16_t offset;
	node->left()->visit(this);
	convert_to_logic(get_type(node->left()));
	bytecode()->addInsn(BC_ILOAD0);
	if (node->kind() == tOR) bytecode()->addInsn(BC_IFICMPNE);
	else bytecode()->addInsn(BC_IFICMPE);
	uint32_t lazy_jump_offset = bytecode()->current();
	bytecode()->addInt16(0);
	bytecode()->addInsn(BC_POP);
	node->right()->visit(this);
	convert_to_logic(get_type(node->right()));
	bytecode()->addInsn(BC_SWAP);
	offset = bytecode()->current() - lazy_jump_offset;
	bytecode()->setInt16(lazy_jump_offset, offset);
	bytecode()->addInsn(BC_POP);
}

void AstTranslator::visit_scope(Scope *scope)
{
	Scope::VarIterator ivar(scope);
	while (ivar.hasNext())
	{
		AstVar *var = ivar.next();
		declare_variable(var, make_pair<uint16_t,uint16_t>(id(), locals()));
		add_local(var->type());
	}
	
	Scope::FunctionIterator ifun(scope);
	while (ifun.hasNext()) declare_function(ifun.next());
	
	ifun = Scope::FunctionIterator(scope);
	while (ifun.hasNext()) ifun.next()->node()->visit(this);
}

uint16_t AstTranslator::size(VarType type) const
{
	switch (type)
	{
	case VT_INT: return sizeof(int64_t);
	case VT_DOUBLE: return sizeof(double);
	case VT_STRING: return sizeof(char const * const);
	default: assert(0);
	}
}

void AstTranslator::declare_variable(AstVar const * const var, std::pair<uint16_t,uint16_t> id)
{
	m_variables.insert(make_pair(var, id));
}

void AstTranslator::declare_function(AstFunction *fun)
{
	BytecodeFunction *bytecode_function = new BytecodeFunction(fun);
	m_code->addFunction(bytecode_function);
	m_functions.insert(make_pair(fun, bytecode_function));
}

void AstTranslator::declare_native(const string& name, const Signature& signature)
{
	uint16_t id = m_code->makeNativeFunction(name, signature, 0);
	m_natives.insert(make_pair(name, id));
}

AstVar *AstTranslator::lookup_variable(std::string const &name)
{
	return scope()->lookupVariable(name);
}

std::pair<uint16_t,uint16_t> AstTranslator::lookup_variable(AstVar const * const var)
{
	return m_variables.find(var)->second;
}

AstFunction *AstTranslator::lookup_function(std::string const &name)
{
	return scope()->lookupFunction(name);
}

uint16_t AstTranslator::lookup_function(AstFunction const * const function)
{
	return m_functions.find(function)->second->id();
}

BytecodeFunction *AstTranslator::lookup_bytecode(AstFunction const * const function)
{
	return m_functions.find(function)->second;
}

uint16_t AstTranslator::lookup_native(std::string const &name)
{
	return m_natives.find(name)->second;
}
