/*
 * codegen.cpp
 *
 *  Created on: Oct 30, 2012
 *      Author: user
 */

#include "bytecodegen.h"
#include <iostream>

using namespace std;

namespace mathvm {

BytecodeFunction* CodeGenVisitor::generate(AstFunction *top) {
	BytecodeFunction* f = new BytecodeFunction(top);
	_current_function = f;
	set_current_scope(top->owner());
	top->node()->body()->visit(this);
	return f;
}

void CodeGenVisitor::visitBinaryOpNode(BinaryOpNode* node) {
	Bytecode* currentBytecode = getCurrentBytecode();
	TokenKind op = node->kind();
	AstNode* left = node->left();
	AstNode* right = node->right();
	VarType type = getNodeType(node);
	switch(op) {
	case tMOD:
		right->visit(this);
		left->visit(this);
		currentBytecode->addInsn(BC_IMOD);
		break;
	case tDIV:
		process_numbers_bin_op(type, left, right, BC_IDIV, BC_DDIV);
		break;
	case tMUL:
		process_numbers_bin_op(type, left, right, BC_IMUL, BC_DMUL);
		break;
	case tSUB:
		process_numbers_bin_op(type, left, right, BC_ISUB, BC_DSUB);
		break;
	case tADD:
		if (type == VT_STRING) {
			// convert to string
			assert(false);
		} else {
			process_numbers_bin_op(type, left, right, BC_IADD, BC_DADD);
		}
		break;
	case tGT:
		process_comprarision(left, right, BC_IFICMPG);
		break;
	case tGE:
		process_comprarision(left, right, BC_IFICMPGE);
		break;
	case tLT:
		process_comprarision(left, right, BC_IFICMPL);
		break;
	case tLE:
		process_comprarision(left, right, BC_IFICMPLE);
		break;
	case tNEQ:
		process_comprarision(left, right, BC_IFICMPNE);
		break;
	case tEQ:
		process_comprarision(left, right, BC_IFICMPE);
		break;
	case tOR:
		process_logic_operation(left, right, BC_IFICMPE);
		break;
	case tAND:
		process_logic_operation(left, right, BC_IFICMPNE);
		break;
	default:
		assert(0);
	}

}

void CodeGenVisitor::visitBlockNode(BlockNode* node) {
	Scope* current_scope = get_current_scope();
	Scope* newScope = node->scope();
	set_current_scope(newScope);
	// process scope
	process_scope(newScope);

	int nodesCount = node->nodes();
	for (int i = 0; i < nodesCount; i++) {
		node->nodeAt(i)->visit(this);
	}

	set_current_scope(current_scope);
}

void CodeGenVisitor::process_scope(Scope* scope) {
	Scope::VarIterator ivar(scope);
	while (ivar.hasNext())
	{
		AstVar *var = ivar.next();
		_variables.insert(make_pair<std::string, uint16_t>(var->name(), get_id()));		
		
	}
	Scope::FunctionIterator ifun(scope);
	while (ifun.hasNext()) {
		AstFunction* fun = ifun.next();
		BytecodeFunction *bytecode_function = new BytecodeFunction(fun);
		_code->addFunction(bytecode_function);
		_functions.insert(make_pair<AstFunction*, BytecodeFunction*>(fun, bytecode_function));
	}
	ifun = Scope::FunctionIterator(scope);
	while (ifun.hasNext()) ifun.next()->node()->visit(this);
}

Scope* CodeGenVisitor::get_current_scope() {
	return _scope;		
}

void CodeGenVisitor::set_current_scope(Scope* scope) {
	_scope = scope;
}

int CodeGenVisitor::get_function_id(AstFunction* astFunction) {
	return _functions.find(astFunction)->second->id();
}

void CodeGenVisitor::store(const AstVar* var, VarType type) {
	
	Bytecode* bytecode = getCurrentBytecode();
	switch (type) {
	case VT_INT:
		{
		bytecode->addInsn(BC_STOREIVAR);
		bytecode->addUInt16(_variables[var->name()]);
		break;
		}
	case VT_DOUBLE:
	{
		bytecode->addInsn(BC_STOREDVAR);
		bytecode->addUInt16(_variables[var->name()]);
		break;
	}
	case VT_STRING:
	{
		bytecode->addInsn(BC_STORESVAR);
		bytecode->addUInt16(_variables[var->name()]);
		break;
	}
	default:
		assert(0);
	}
}
 
void CodeGenVisitor::visitCallNode(CallNode* node) {

	AstFunction* f = get_current_scope()->lookupFunction(node->name(), true);
	int id = get_function_id(f);
	int paramsCount = node->parametersNumber();
	Bytecode* currentBytecode = getCurrentBytecode();
	for (int i = 0; i < paramsCount; i++) {
		AstNode* parameter = node->parameterAt(i);
		parameter->visit(this);
		if ((f->parameterType(i) == VT_DOUBLE) && (getNodeType(parameter) == VT_INT)) {
			currentBytecode->addInsn(BC_I2D);
		}
	}
	currentBytecode->add(BC_CALL);
	currentBytecode->addInt16(id);

}

void CodeGenVisitor::visitForNode(ForNode* node) {
	BinaryOpNode* inExpr = node->inExpr()->asBinaryOpNode();
	inExpr->left()->visit(this);
	store(node->var(), VT_INT);
	Bytecode* currentBytecode = getCurrentBytecode();
	int loop_condition = currentBytecode->current();
	load_var(node->var());
	inExpr->right()->visit(this);
	currentBytecode->addInsn(BC_IFICMPLE);
	int if_false = currentBytecode->current();
	currentBytecode->addInt16(0);
	node->body()->visit(this);
	currentBytecode->addInsn(BC_JA);
	currentBytecode->add(loop_condition - currentBytecode->current());
	currentBytecode->setInt16(if_false, currentBytecode->current() - if_false);
}

void CodeGenVisitor::visitFunctionNode(FunctionNode* node) {
	AstFunction* function = get_current_scope()->lookupFunction(node->name());
	BytecodeFunction* prevBytecode = _current_function;
	_current_function = _functions.at(function);
	Scope* scope = node->body()->scope()->parent();
	process_scope(scope);
	for (uint32_t i = 0; i != node->parametersNumber(); ++i) {
		AstVar const * const var = scope->lookupVariable((node->parameterName(node->parametersNumber() - i - 1)));
		store(var, var->type());
	}
	node->body()->visit(this);
	_current_function = prevBytecode;
}

Bytecode* CodeGenVisitor::getCurrentBytecode() {
	return _current_function->bytecode();
}

void CodeGenVisitor::visitIfNode(IfNode* node) {
	node->ifExpr()->visit(this);
	Bytecode* currentBytecode = getCurrentBytecode();
	currentBytecode->addInsn(BC_ILOAD0);
	currentBytecode->addInsn(BC_IFICMPE);
	int if_false = currentBytecode->current();
	currentBytecode->addInt16(0);
	// if not false
	node->thenBlock()->visit(this);
	currentBytecode->addInsn(BC_JA);
	int if_true = currentBytecode->current();
	currentBytecode->addInt16(0);
	currentBytecode->setInt16(if_false, currentBytecode->current() - if_false);
	// if false
	if (node->elseBlock() && node->elseBlock()->nodes()) {		
		node->elseBlock()->visit(this);
	}
	currentBytecode->setInt16(if_true, currentBytecode->current() - if_true);
}

void CodeGenVisitor::visitNativeCallNode(NativeCallNode* node) {

}

void CodeGenVisitor::visitPrintNode(PrintNode* node) {
	int operandsCount = node->operands();
	Bytecode* currentBytecode = getCurrentBytecode();
	for (int i = 0; i < operandsCount; i++) {
		AstNode* operand = node->operandAt(i);
		operand->visit(this);
		switch(getNodeType(operand)) {
		case VT_INT:
			currentBytecode->addInsn(BC_IPRINT);
			break;
		case VT_DOUBLE:
			currentBytecode->addInsn(BC_DPRINT);
			break;
		case VT_STRING:
			currentBytecode->addInsn(BC_SPRINT);
			break;
		default:
			assert(0);
		}
	}
}

void CodeGenVisitor::visitReturnNode(ReturnNode* node) {
	VarType type = getNodeType(node);
	Bytecode* currentBytecode = getCurrentBytecode();
	if (type != VT_VOID) {
		node->returnExpr()->visit(this);
		if ((type == VT_DOUBLE) && (getNodeType(node->returnExpr()) == VT_INT)) {
			currentBytecode->addInsn(BC_I2D);
		}
	}
	currentBytecode->addInsn(BC_RETURN);
}

void CodeGenVisitor::visitUnaryOpNode(UnaryOpNode* node) {
	node->operand()->visit(this);
	Bytecode* currentBytecode = getCurrentBytecode();
	switch(node->kind()) {
	case tNOT:
	{
		convert_to_boolean(node);
		currentBytecode->addInsn(BC_ILOAD0);
		currentBytecode->addInsn(BC_IFICMPE);
		int current = currentBytecode->current();
		currentBytecode->addInt16(0);
		currentBytecode->addInsn(BC_ILOAD0);
		currentBytecode->setInt16(current, currentBytecode->current() - current);
		currentBytecode->addInsn(BC_ILOAD1);		
		break;
	}
	case tSUB:
		switch(getNodeType(node->operand())){
		case VT_INT:
			currentBytecode->addInsn(BC_INEG);
			break;
		case VT_DOUBLE:
			currentBytecode->addInsn(BC_DNEG);
			break;
		default:
			assert(0);
		}
		break;
	default:
		assert(0);
 	}
}

void CodeGenVisitor::visitWhileNode(WhileNode* node) {
	Bytecode* currentBytecode = getCurrentBytecode();
	int if_position = currentBytecode->current();
	currentBytecode->addInsn(BC_ILOAD0);
	node->whileExpr()->visit(this);
	convert_to_boolean(node->whileExpr());
	currentBytecode->addInsn(BC_IFICMPE);
	int if_false_position = currentBytecode->current();
	currentBytecode->addInt16(0);
	node->loopBlock()->visit(this);
	currentBytecode->addInsn(BC_JA);
	currentBytecode->addInt16(if_position - currentBytecode->current());
	//currentBytecode->addInsn(BC_POP);
	currentBytecode->setInt16(if_false_position, currentBytecode->current() - if_false_position);
}

void CodeGenVisitor::visitStringLiteralNode(StringLiteralNode* node) {
	load_string_const(node->literal());
}

void CodeGenVisitor::visitIntLiteralNode(IntLiteralNode* node) {
	load_int_const(node->literal());
}

void CodeGenVisitor::visitDoubleLiteralNode(DoubleLiteralNode* node) {
	load_double_const(node->literal());
}

void CodeGenVisitor::visitLoadNode(LoadNode* node) {
	load_var(node->var());
}

void CodeGenVisitor::load_var(const AstVar* var) {
	Bytecode* bytecode = getCurrentBytecode();
	switch (var->type()) {
	case VT_INT:
	{
		bytecode->addInsn(BC_LOADIVAR);
		bytecode->addUInt16(_variables[var->name()]);
		break;
	}
	case VT_DOUBLE:
	{
		bytecode->addInsn(BC_LOADDVAR);
		bytecode->addUInt16(_variables[var->name()]);
		break;
	}
	case VT_STRING:
	{
		bytecode->addInsn(BC_LOADSVAR);
		bytecode->addUInt16(_variables[var->name()]);
		break;
	}
	default:
		assert(0);
	}
	
}

void CodeGenVisitor::visitStoreNode(StoreNode* node) {;
	switch(node->op()) {
	case tASSIGN:
		node->value()->visit(this);
		break;
	case tINCRSET:
		process_numbers_bin_op(getNodeType(node), new LoadNode(0, node->var()), node->value(), BC_IADD, BC_DADD);
		break;
	case tDECRSET:
		process_numbers_bin_op(getNodeType(node), new LoadNode(0, node->var()), node->value(), BC_ISUB, BC_DSUB);
		break;
	default:
		assert(0);	
	}
	store(node->var(), getNodeType(node->value()));
}

void CodeGenVisitor::load_string_const(const string& value) {
	Bytecode* currentBytecode = getCurrentBytecode();
	if (value.empty()) {
		currentBytecode->addInsn(BC_SLOAD0);
	} else {
		currentBytecode->addInsn(BC_SLOAD);
		currentBytecode->addInt16(_code->makeStringConstant(value));
	}
}

void CodeGenVisitor::load_int_const(int64_t value) {
	Bytecode* bc = getCurrentBytecode();
	if (value == 1) {
		bc->addInsn(BC_ILOAD1);
	} else if (value == -1) {
		bc->addInsn(BC_ILOADM1);
	} else if (value == 0) {
		bc->addInsn(BC_ILOAD0);
	} else {
		bc->addInsn(BC_ILOAD);
		bc->addTyped(value);
	}
}

void CodeGenVisitor::load_double_const(double value) {
	Bytecode* bc = getCurrentBytecode();
	if (value == 1.0) {
		bc->addInsn(BC_DLOAD1);
	} else if (value == -1.0) {
		bc->addInsn(BC_DLOADM1);
	} else if (value == 0.0) {
		bc->addInsn(BC_DLOAD0);
	} else {
		bc->addInsn(BC_DLOAD);
		bc->addTyped(value);
	}
}

void CodeGenVisitor::process_numbers_bin_op(VarType commonType, AstNode* left, AstNode* right, Instruction ifInt, Instruction ifDouble) {
	Bytecode* currentBytecode = getCurrentBytecode();
	if (commonType == VT_INT) {

		right->visit(this);
		left->visit(this);
		currentBytecode->addInsn(ifInt);
	} else if (commonType == VT_DOUBLE) {
		right->visit(this);
		if (getNodeType(right) == VT_INT) {
			currentBytecode->addInsn(BC_I2D);
		}
		left->visit(this);
		if (getNodeType(left) == VT_INT) {
			currentBytecode->addInsn(BC_I2D);
		}
		currentBytecode->addInsn(ifDouble);
	} else {
		assert(0);
	}
}

void CodeGenVisitor::process_comprarision(AstNode* left, AstNode* right, Instruction comprassion) {
	VarType leftType = getNodeType(left);
	VarType rightType = getNodeType(right);
	Bytecode* currentBytecode = getCurrentBytecode();
	if ((leftType == VT_INT) && (rightType == VT_INT)) {
		right->visit(this);
		left->visit(this);
		currentBytecode->addInsn(BC_ICMP);
	} else {
		right->visit(this);
		if (rightType == VT_INT) {
			currentBytecode->addInsn(BC_I2D);
		}
		left->visit(this);
		if (leftType == VT_DOUBLE) {
			currentBytecode->addInsn(BC_I2D);
		}
		currentBytecode->addInsn(BC_DCMP);
	}
	currentBytecode->addInsn(BC_ILOAD0);
	//currentBytecode->addInsn(BC_SWAP);

	currentBytecode->addInsn(comprassion);
	int comprassion_position = currentBytecode->current();
	currentBytecode->addInt16(0);
	//currentBytecode->addInsn(BC_POP);
	//currentBytecode->addInsn(BC_POP);
	currentBytecode->addInsn(BC_ILOAD0);
	currentBytecode->addInsn(BC_JA);
	int jumb_after_true_position = currentBytecode->current();
	currentBytecode->addInt16(0);
	currentBytecode->setInt16(comprassion_position, currentBytecode->current() - comprassion_position);
	//currentBytecode->addInsn(BC_POP);
	//currentBytecode->addInsn(BC_POP);
	currentBytecode->addInsn(BC_ILOAD1);
	currentBytecode->setInt16(jumb_after_true_position, currentBytecode->current() - jumb_after_true_position);
}

void CodeGenVisitor::process_logic_operation(AstNode* left, AstNode* right, Instruction operation) {
	Bytecode* currentBytecode = getCurrentBytecode();
	left->visit(this);
	convert_to_boolean(left);
	currentBytecode->addInsn(BC_ILOAD0);
	currentBytecode->addInsn(operation);
	int check_left = currentBytecode->current();
	currentBytecode->addInt16(0);
	// if left is not 0
	//currentBytecode->addInsn(BC_POP);
	currentBytecode->addInsn(BC_ILOAD0);
	currentBytecode->addInsn(BC_JA);
	int set_result_if_left_is_true = currentBytecode->current();
	currentBytecode->addInt16(0);

	// set where to jump if left is 0
	currentBytecode->setInt16(check_left, currentBytecode->current() - check_left);
	// if left is 0
	//currentBytecode->addInsn(BC_POP);
	//currentBytecode->addInsn(BC_POP);
	right->visit(this);
	convert_to_boolean(right);

	// set where to jump if left is 1
	currentBytecode->setInt16(set_result_if_left_is_true, currentBytecode->current() - set_result_if_left_is_true);
}

void CodeGenVisitor::convert_to_boolean(AstNode* node) {
	Bytecode* currentBytecode = getCurrentBytecode();
	if (getNodeType(node) == VT_DOUBLE) {
		currentBytecode->addInsn(BC_DLOAD0);
		currentBytecode->addInsn(BC_DCMP);
	}
}


}


