#include "bc_translator.h"

using namespace mathvm;

#include <iostream>


BytecodeTranslator::BytecodeTranslator(AstFunction* ast_top):
		_ast_top(ast_top), _code(ast_top), _bytecode(_code.bytecode()), lastId(0) {}

BytecodeTranslator::~BytecodeTranslator() {};


CodeImpl* BytecodeTranslator::run() {
	_ast_top->node()->visit(this);
	return &_code;
}


void BytecodeTranslator::visitStringLiteralNode(StringLiteralNode *node) {
	_bytecode->addInsn(BC_SLOAD);
	_bytecode->addInt16(_code.makeStringConstant(node->literal()));
	stackTypes.push_back(VT_STRING);
}
void BytecodeTranslator::visitDoubleLiteralNode(DoubleLiteralNode *node) {
	_bytecode->addInsn(BC_DLOAD);
	_bytecode->addDouble(node->literal());
	stackTypes.push_back(VT_DOUBLE);
}
void BytecodeTranslator::visitIntLiteralNode(IntLiteralNode *node) {
	_bytecode->addInsn(BC_ILOAD);
	_bytecode->addInt64(node->literal());
	stackTypes.push_back(VT_INT);
}
void BytecodeTranslator::visitLoadNode(LoadNode *node) {
	switch (node->var()->type())
	{
	case VT_INT:
	{
		stackTypes.push_back(VT_INT);
		_bytecode->addInsn(BC_LOADIVAR);
		_bytecode->addUInt16(vars[node->var()->name()]);
		break;
	}
	case VT_DOUBLE:
	{
		stackTypes.push_back(VT_DOUBLE);
		_bytecode->addInsn(BC_LOADDVAR);
		_bytecode->addUInt16(vars[node->var()->name()]);
		break;
	}
	case VT_STRING:
	{
		stackTypes.push_back(VT_STRING);
		_bytecode->addInsn(BC_LOADSVAR);
		_bytecode->addUInt16(vars[node->var()->name()]);
		break;
	}
	default:
	{
		std::cout << "Can't load variable!\n";
	}
	}
	node->var()->name();
}
void BytecodeTranslator::visitStoreNode(StoreNode *node) {
	node->value()->visit(this);
	switch (node->op())
	{
	case tINCRSET:
	{
		switch (node->var()->type())
		{
		case VT_INT:
		{
			_bytecode->addInsn(BC_LOADIVAR);
			_bytecode->addUInt16(vars[node->var()->name()]);
			_bytecode->addInsn(BC_IADD);
			_bytecode->addInsn(BC_STOREIVAR);
			_bytecode->addUInt16(vars[node->var()->name()]);
			break;
		}
		case VT_DOUBLE:
		{
			_bytecode->addInsn(BC_LOADDVAR);
			_bytecode->addUInt16(vars[node->var()->name()]);
			_bytecode->addInsn(BC_DADD);
			_bytecode->addInsn(BC_STOREDVAR);
			_bytecode->addUInt16(vars[node->var()->name()]);
			break;
		}
		default:
		{
			cerr << "Can't store variable!" << endl;
		}
		}
		break;
	}
	case tDECRSET:
	{
		switch (node->var()->type())
		{
		case VT_INT:
		{
			_bytecode->addInsn(BC_LOADIVAR);
			_bytecode->addUInt16(vars[node->var()->name()]);
			_bytecode->addInsn(BC_ISUB);
			_bytecode->addInsn(BC_STOREIVAR);
			_bytecode->addUInt16(vars[node->var()->name()]);
			break;
		}
		case VT_DOUBLE:
		{
			_bytecode->addInsn(BC_LOADDVAR);
			_bytecode->addUInt16(vars[node->var()->name()]);
			_bytecode->addInsn(BC_DSUB);
			_bytecode->addInsn(BC_STOREDVAR);
			_bytecode->addUInt16(vars[node->var()->name()]);
			break;
		}
		default:
		{
			cerr << "Can't store variable!" << endl;
		}
		}
		break;
	}
	case tASSIGN:
	{
		switch (node->var()->type())
		{
		case VT_INT:
		{
			_bytecode->addInsn(BC_STOREIVAR);
			_bytecode->addUInt16(vars[node->var()->name()]);
			break;
		}
		case VT_DOUBLE:
		{
			_bytecode->addInsn(BC_STOREDVAR);
			_bytecode->addUInt16(vars[node->var()->name()]);
			break;
		}
		case VT_STRING:
		{
			if (stackTypes.back() != VT_STRING)
			{
				cerr << "Can't cast to string!" << endl;
			}
			_bytecode->addInsn(BC_STORESVAR);
			_bytecode->addUInt16(vars[node->var()->name()]);
			break;
		}
		default:
		{
			cerr << "Can't store variable!" << endl;
		}
		}
		break;
	}
	default:
	{
		cerr << "Can't store variable!" << endl;
	}
	}
}

void BytecodeTranslator::visitForNode(ForNode *node) {
	int indexId = vars[node->var()->name()];

	BinaryOpNode *range = (BinaryOpNode *) node->inExpr();
	range->left()->visit(this);
	_bytecode->addInsn(BC_STOREIVAR);
	_bytecode->addUInt16(indexId);

	Label forLabel(_bytecode);
	Label endLabel(_bytecode);
	_bytecode->bind(forLabel);

	range->right()->visit(this);
	_bytecode->addInsn(BC_LOADIVAR);
	_bytecode->addUInt16(indexId);
	_bytecode->addBranch(BC_IFICMPG, endLabel);
	node->body()->visit(this);

	_bytecode->addInsn(BC_LOADIVAR);
	_bytecode->addUInt16(indexId);
	_bytecode->addInsn(BC_ILOAD1);
	_bytecode->addInsn(BC_IADD);
	_bytecode->addInsn(BC_STOREIVAR);
	_bytecode->addUInt16(indexId);

	_bytecode->addBranch(BC_JA, forLabel);
	_bytecode->bind(endLabel);
}

void BytecodeTranslator::visitWhileNode(WhileNode *node) {
	Label while_label(_bytecode);
	Label end(_bytecode);
	_bytecode->bind(while_label);
	node->whileExpr()->visit(this);
	_bytecode->addInsn(BC_ILOAD1);
	_bytecode->addBranch(BC_IFICMPNE, end);
	node->loopBlock()->visit(this);
	_bytecode->addBranch(BC_JA, while_label);
	_bytecode->bind(end);
}

void BytecodeTranslator::visitIfNode(IfNode *node) {
	node->ifExpr()->visit(this);
	Label else_label(_bytecode);
	Label end(_bytecode);
	_bytecode->addInsn(BC_ILOAD1);
	_bytecode->addBranch(BC_IFICMPNE, else_label);
	node->thenBlock()->visit(this);
	_bytecode->addBranch(BC_JA,end);
	_bytecode->bind(else_label);
	if (node->elseBlock())
	{
		node->elseBlock()->visit(this);
	}
	_bytecode->bind(end);
}

void BytecodeTranslator::visitBlockNode(BlockNode *node) {

	map<string, int> before;
	Scope::VarIterator var(node->scope());

	//allocate variables
	while(var.hasNext()) {
		AstVar* cur_var = var.next();
		vars[cur_var->name()] = lastId++;
	}
	Scope::FunctionIterator func(node->scope());
	while(func.hasNext())
	{
		AstFunction* function = func.next();
		BytecodeFunction* bytecodeFunction = new BytecodeFunction(function);
		_code.addFunction(bytecodeFunction);
		uint32_t i = _bytecode->length();
		function->node()->visit(this);
		i = i + 3;
		while(i < _bytecode->length()) {
			bytecodeFunction->bytecode()->add(_bytecode->get(i));
			++i;
		}
	}
	for (uint32_t i = 0; i < node->nodes(); ++i)
	{
		node->nodeAt(i)->visit(this);
	}

}

void BytecodeTranslator::visitFunctionNode(FunctionNode *node) {

	if(node->name() == "<top>")
	{
		node->body()->visit(this);
		return;
	}
	Label end(_bytecode);
	_bytecode->addBranch(BC_JA, end);
	for (uint32_t j = 0; j < node->parametersNumber(); j++) {
		_bytecode->addInsn(BC_STOREIVAR);
		_bytecode->addUInt16(lastId);
		vars[node->parameterName(j)] = lastId++;
	}
	if (node->body()->nodes() > 0 && node->body()->nodeAt(0)->isNativeCallNode())
	{
		node->body()->nodeAt(0)->visit(this);
	}
	else
	{
		node->body()->visit(this);
	}
	stackTypes.push_back(node->returnType());
	_bytecode->bind(end);
}

void BytecodeTranslator::visitReturnNode(ReturnNode *node) {
	if(node->returnExpr()) {
		node->returnExpr()->visit(this);
	}
	_bytecode->addInsn(BC_RETURN);
}

void BytecodeTranslator::visitCallNode(CallNode *node) {
	for (uint32_t i = 0; i < node->parametersNumber(); i++) {
		node->parameterAt(i)->visit(this);
	}
	stackTypes.push_back(_code.functionByName(node->name())->returnType());
	_bytecode->addInsn(BC_CALL);
	_bytecode->addUInt16(_code.functionByName(node->name())->id());
}

void BytecodeTranslator::visitNativeCallNode(NativeCallNode *node) {
	_bytecode->addInsn(BC_CALLNATIVE);
	_bytecode->addUInt16(_code.makeNativeFunction(node->nativeName(), node->nativeSignature(), 0));
}

void BytecodeTranslator::visitPrintNode(PrintNode *node) {
	for (uint32_t i = 0; i < node->operands(); i++) {
		AstNode *pNode = node->operandAt(i);
		pNode->visit(this);
		switch (stackTypes.back())
		{
		case VT_INT:
		{
			_bytecode->addInsn(BC_IPRINT);
			break;
		}
		case VT_DOUBLE:
		{
			_bytecode->addInsn(BC_DPRINT);
			break;
		}
		case VT_STRING:
		{
			_bytecode->addInsn(BC_SPRINT);
			break;
		}
		default:
			cerr << "Can't print properly!" << endl;
		}
	}
}

void BytecodeTranslator::castStackToInt() {
	if (stackTypes.back() == VT_INT)
	{
		return;
	}
	else if (stackTypes.back() == VT_DOUBLE)
	{
		_bytecode->addInsn(BC_D2I);
	}
	else if (stackTypes.back() == VT_STRING)
	{
		_bytecode->addInsn(BC_S2I);
	}
	stackTypes.back() = VT_INT;

}

void BytecodeTranslator::castStackToDouble() {
	if (stackTypes.back() == VT_DOUBLE)
	{
		return;
	}
	else if (stackTypes.back() == VT_INT)
	{
		_bytecode->addInsn(BC_I2D);
	}
	else if (stackTypes.back() == VT_STRING)
	{
		_bytecode->addInsn(BC_S2I);
		_bytecode->addInsn(BC_I2D);
	}
	stackTypes.back() = VT_DOUBLE;

}

void BytecodeTranslator::castOperands(bool toInt) {
	if (stackTypes.size() < 2)
	{
		std::cout << "Can't process binary operation!";
	}
	if ((stackTypes.at(stackTypes.size() - 2) == VT_INT && stackTypes.back() == VT_INT)
			|| (!toInt && stackTypes.at(stackTypes.size() - 2) == VT_DOUBLE && stackTypes.back() == VT_DOUBLE))
	{
		return;
	}
	if (!toInt && (stackTypes.back() == VT_DOUBLE || stackTypes.at(stackTypes.size() - 2) == VT_DOUBLE))
	{
		VarType toChange = stackTypes.back();
		if (stackTypes.back() == VT_DOUBLE)
		{
			_bytecode->addInsn(BC_SWAP);
			toChange = stackTypes.at(stackTypes.size() - 2);
		}
		if (toChange == VT_STRING)
		{
			_bytecode->addInsn(BC_S2I);
		}
		_bytecode->addInsn(BC_I2D);
		if (stackTypes.back() == VT_DOUBLE)
		{
			_bytecode->addInsn(BC_SWAP);
		}
		stackTypes.back() = VT_DOUBLE;
		stackTypes.at(stackTypes.size() - 2) = VT_DOUBLE;
		return;
	}
	else
	{
		if (stackTypes.back() == VT_STRING)
		{
			_bytecode->addInsn(BC_S2I);
		}
		if (stackTypes.back() == VT_DOUBLE)
		{
			_bytecode->addInsn(BC_D2I);
		}
		if (stackTypes.at(stackTypes.size() - 2) != VT_INT)
		{
			_bytecode->addInsn(BC_SWAP);
			if (stackTypes.at(stackTypes.size() - 2) == VT_STRING)
			{
				_bytecode->addInsn(BC_S2I);
			}
			if (stackTypes.at(stackTypes.size() - 2) == VT_DOUBLE)
			{
				_bytecode->addInsn(BC_D2I);
			}
			_bytecode->addInsn(BC_SWAP);
		}
		stackTypes.back() = VT_INT;
		stackTypes.at(stackTypes.size() - 2) = VT_INT;
	}

}

void BytecodeTranslator::visitBinaryOpNode(BinaryOpNode *node) {

	node->left()->visit(this);
	node->right()->visit(this);

	switch(node->kind())
	{
	case tAND:
	{
		castOperands(1);
		Label right(_bytecode);
		Label true_end(_bytecode);
		Label end(_bytecode);
		_bytecode->addInsn(BC_ILOAD0);
		_bytecode->addBranch(BC_IFICMPNE, right);
		_bytecode->addInsn(BC_POP);
		_bytecode->addInsn(BC_ILOAD0);
		_bytecode->bind(right);
		_bytecode->addInsn(BC_ILOAD0);
		_bytecode->addBranch(BC_IFICMPNE, true_end);
		_bytecode->addInsn(BC_ILOAD0);
		_bytecode->addBranch(BC_JA, end);
		_bytecode->bind(true_end);
		_bytecode->addInsn(BC_ILOAD1);
		_bytecode->bind(end);
		break;
	}
	case tOR:
	{
		castOperands(1);
		Label true_end_pop(_bytecode);
		Label true_end(_bytecode);
		Label end(_bytecode);
		_bytecode->addInsn(BC_ILOAD0);
		_bytecode->addBranch(BC_IFICMPNE, true_end_pop);
		_bytecode->addInsn(BC_ILOAD0);
		_bytecode->addBranch(BC_IFICMPNE, true_end);
		_bytecode->addInsn(BC_ILOAD0);
		_bytecode->addBranch(BC_JA, end);
		_bytecode->bind(true_end_pop);
		_bytecode->addInsn(BC_POP);
		_bytecode->bind(true_end);
		_bytecode->addInsn(BC_ILOAD1);
		_bytecode->bind(end);
		break;
	}
	case tEQ:
	{
		castOperands(0);
		Label true_end(_bytecode);
		Label end(_bytecode);
		_bytecode->addBranch(BC_IFICMPE, true_end);
		_bytecode->addInsn(BC_ILOAD0);
		_bytecode->addBranch(BC_JA, end);
		_bytecode->bind(true_end);
		_bytecode->addInsn(BC_ILOAD1);
		_bytecode->bind(end);
		break;
	}
	case tNEQ:
	{
		castOperands(0);
		_bytecode->add(BC_SWAP);
		Label true_end(_bytecode);
		Label end(_bytecode);
		_bytecode->addBranch(BC_IFICMPNE, true_end);
		_bytecode->addInsn(BC_ILOAD0);
		_bytecode->addBranch(BC_JA, end);
		_bytecode->bind(true_end);
		_bytecode->addInsn(BC_ILOAD1);
		_bytecode->bind(end);
		break;
	}
	case tGT:
	{
		castOperands(0);
		_bytecode->add(BC_SWAP);
		Label true_end(_bytecode);
		Label end(_bytecode);
		_bytecode->addBranch(BC_IFICMPG, true_end);
		_bytecode->addInsn(BC_ILOAD0);
		_bytecode->addBranch(BC_JA, end);
		_bytecode->bind(true_end);
		_bytecode->addInsn(BC_ILOAD1);
		_bytecode->bind(end);
		break;
	}
	case tGE:
	{
		castOperands(0);
		_bytecode->add(BC_SWAP);
		Label true_end(_bytecode);
		Label end(_bytecode);
		_bytecode->addBranch(BC_IFICMPGE, true_end);
		_bytecode->addInsn(BC_ILOAD0);
		_bytecode->addBranch(BC_JA, end);
		_bytecode->bind(true_end);
		_bytecode->addInsn(BC_ILOAD1);
		_bytecode->bind(end);
		break;
	}
	case tLT:
	{
		castOperands(0);
		_bytecode->add(BC_SWAP);
		Label true_end(_bytecode);
		Label end(_bytecode);
		_bytecode->addBranch(BC_IFICMPL, true_end);
		_bytecode->addInsn(BC_ILOAD0);
		_bytecode->addBranch(BC_JA, end);
		_bytecode->bind(true_end);
		_bytecode->addInsn(BC_ILOAD1);
		_bytecode->bind(end);
		break;
	}
	case tLE:
	{
		castOperands(0);
		_bytecode->add(BC_SWAP);
		Label true_end(_bytecode);
		Label end(_bytecode);
		_bytecode->addBranch(BC_IFICMPLE, true_end);
		_bytecode->addInsn(BC_ILOAD0);
		_bytecode->addBranch(BC_JA, end);
		_bytecode->bind(true_end);
		_bytecode->addInsn(BC_ILOAD1);
		_bytecode->bind(end);
		break;
	}

	case tADD:
	{
		castOperands(0);
		if (stackTypes.back() == VT_DOUBLE)
		{
			_bytecode->addInsn(BC_DADD);
		}
		else if (stackTypes.back() == VT_INT)
		{
			_bytecode->addInsn(BC_IADD);
		}
		else
		{
			cerr << "Invalid addition!" << endl;
		}
		break;
	}
	case tSUB:
	{
		castOperands(0);
		_bytecode->add(BC_SWAP);
		if (stackTypes.back() == VT_DOUBLE)
		{
			_bytecode->addInsn(BC_DSUB);
		}
		else if (stackTypes.back() == VT_INT)
		{
			_bytecode->addInsn(BC_ISUB);
		}
		else
		{
			cerr << "Invalid subtraction!" << endl;
		}
		break;
	}
	case tMUL:
	{
		castOperands(0);
		if (stackTypes.back() == VT_DOUBLE)
		{
			_bytecode->addInsn(BC_DMUL);
		}
		else if (stackTypes.back() == VT_INT)
		{
			_bytecode->addInsn(BC_IMUL);
		}
		else
		{
			cerr << "Invalid multiplication!" << endl;
		}
		break;
	}
	case tDIV:
	{
		castOperands(0);
		_bytecode->add(BC_SWAP);
		if (stackTypes.back() == VT_DOUBLE)
		{
			_bytecode->addInsn(BC_DDIV);
		}
		else if (stackTypes.back() == VT_INT)
		{
			_bytecode->addInsn(BC_IDIV);
		}
		else
		{
			cerr << "Invalid division!" << endl;
		}
		break;
	}
	case tMOD:
	{
		castOperands(1);
		_bytecode->add(BC_SWAP);
		if (stackTypes.back() == VT_INT)
		{
			_bytecode->addInsn(BC_IMOD);
		}
		else
		{
			cerr << "Invalid mod!" << endl;
		}
		break;
	}

	case tAOR:
	{
		castOperands(1);
		_bytecode->addInsn(BC_IAOR);
		break;
	}

	case tAAND:
	{
		castOperands(1);
		_bytecode->addInsn(BC_IAAND);
		break;
	}

	case tAXOR:
	{
		castOperands(1);
		_bytecode->addInsn(BC_IAXOR);
		break;
	}


	default:
	{
		cerr << "Invalid binary operation!" << endl;
	}
	}
}

void BytecodeTranslator::visitUnaryOpNode(UnaryOpNode *node) {


	node->operand()->visit(this);
	switch(node->kind())
	{
	case tNOT:
	{
		Label true_end(_bytecode);
		Label end(_bytecode);
		_bytecode->addInsn(BC_ILOAD0);
		_bytecode->addBranch(BC_IFICMPNE, true_end);
		_bytecode->addInsn(BC_ILOAD1);
		_bytecode->addBranch(BC_JA, end);
		_bytecode->bind(true_end);
		_bytecode->addInsn(BC_ILOAD0);
		_bytecode->bind(end);
		break;
	}
	case tSUB:
	{
		if (stackTypes.back() == VT_DOUBLE)
		{
			_bytecode->addInsn(BC_DNEG);
		}
		else if (stackTypes.back() == VT_INT)
		{
			_bytecode->addInsn(BC_INEG);
		}
		else
		{
			cerr << "Invalid negating!" << endl;
		}
		break;
	}
	default:
	{
		cerr << "Invalid unary operation!" << endl;
	}
	}
}

