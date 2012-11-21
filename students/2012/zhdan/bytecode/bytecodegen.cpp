/*
 * codegen.cpp
 *
 *  Created on: Oct 30, 2012
 *      Author: user
 */

#include "bytecodegen.h"
#include "mathvm.h"
#include "ast.h"

using namespace std;

namespace mathvm {

void CodeGenVisitor::visitBinaryOpNode(BinaryOpNode* node) {
	Bytecode* currentBytecode = getCurrentBytecode();
	TokenKind* op = node->kind();
	AstNode* left = node->left();
	AstNode* right = node->right();
	VarType type = *getNodeType(node);
	VarType leftType = *getNodeType(left);
	VarType rightType = *getNodeType(right);
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
	case tEQ:
		process_logic_operation(left, right, BC_IFICMPNE);
		break;
	}

}

void CodeGenVisitor::visitBlockNode(BlockNode* node) {
	Scope* current_scope = get_curernt_scope();
	set_current_scope(node->scope());
	int nodesCount = node->nodes();
	for (int i = 0; i < nodesCount; i++) {
		node->nodeAt(i)->visit(this);
	}
	set_current_scope(current_scope);
}

void CodeGenVisitor::visitCallNode(CallNode* node) {
	AstFunction* f = get_curernt_scope()->lookupFunction(node->name(), true);
	int id = get_function_id(f);
	int paramsCount = node->parametersNumber();
	Bytecode* currentBytecode = getCurrentBytecode();
	for (int i = 0; i < paramsCount; i++) {
		AstNode* parameter = node->parameterAt(i);
		parameter->visit(this);
		if ((f->parameterType(i) == VT_DOUBLE) && (*getNodeType(parameter) == VT_INT)) {
			currentBytecode->addInsn(BC_I2D);
		}
	}
	currentBytecode->add(BC_CALL);
	currentBytecode->addInt16(id);
}

void CodeGenVisitor::visitForNode(ForNode* node) {

}

void CodeGenVisitor::visitFunctionNode(FunctionNode* node) {

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
	node->elseBlock()->visit(this);
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
		switch(*getNodeType(operand)) {
		case VT_INT:
			currentBytecode->addInsn(BC_IPRINT);
			break;
		case VT_DOUBLE:
			currentBytecode->addInsn(BC_DPRINT);
			break;
		case VT_STRING:
			currentBytecode->addInsn(BC_SPRINT);
			break;
		}
	}
}

void CodeGenVisitor::visitReturnNode(ReturnNode* node) {
	VarType type = *getNodeType(node);
	Bytecode* currentBytecode = getCurrentBytecode();
	if (type != VT_VOID) {
		node->returnExpr()->visit(this);
		if ((type == VT_DOUBLE) && (*getNodeType(node->returnExpr()))) {
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
		convert_to_boolean(node);
		currentBytecode->addInsn(BC_ILOAD0);
		currentBytecode->addInsn(BC_ICMP);
		break;
	case tSUB:
		switch(*getNodeType(node->operand())){
		case VT_INT:
			currentBytecode->addInsn(BC_INEG);
			break;
		case VT_DOUBLE:
			currentBytecode->addInsn(BC_DNEG);
			break;
		}
		break;
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
	currentBytecode->addInsn(BC_POP);
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

}

void CodeGenVisitor::visitStoreNode(StoreNode* node) {
	Bytecode* currentBytecode = getCurrentBytecode();
	switch(node->op()) {
	case tASSIGN:
		node->value()->visit(this);
		break;
	case tINCRSET:
		process_numbers_bin_op(*getNodeType(node), new LoadNode(0, node->var()), node->value(), BC_IADD, BC_DADD);
		break;
	case tDECRSET:
		process_numbers_bin_op(*getNodeType(node), new LoadNode(0, node->var()), node->value(), BC_ISUB, BC_DSUB);
		break;
	}

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

void CodeGenVisitor::load_int_const(int value) {
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
		if (*getNodeType(right) == VT_INT) {
			currentBytecode->addInsn(BC_I2D);
		}
		left->visit(this);
		if (*getNodeType(left) == VT_INT) {
			currentBytecode->addInsn(BC_I2D);
		}
		currentBytecode->addInsn(BC_DMUL);
	} else {
		assert(0);
	}
}

void CodeGenVisitor::process_comprarision(AstNode* left, AstNode* right, Instruction comprassion) {
	VarType leftType = *getNodeType(left);
	VarType rightType = *getNodeType(right);
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
	currentBytecode->addInsn(BC_SWAP);

	currentBytecode->addInsn(comprassion);
	int comprassion_position = currentBytecode->current();
	currentBytecode->addInt16(0);
	currentBytecode->addInsn(BC_POP);
	currentBytecode->addInsn(BC_POP);
	currentBytecode->addInsn(BC_ILOAD0);
	currentBytecode->addInsn(BC_JA);
	int jumb_after_true_position = currentBytecode->current();
	currentBytecode->addInt16(0);
	currentBytecode->setInt16(comprassion_position, currentBytecode->current() - comprassion_position);
	currentBytecode->addInsn(BC_POP);
	currentBytecode->addInsn(BC_POP);
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
	currentBytecode->addInsn(BC_POP);
	currentBytecode->addInsn(BC_JA);
	int set_result_if_left_is_true = currentBytecode->current();
	currentBytecode->addInt16(0);

	// set where to jump if left is 0
	currentBytecode->setInt16(check_left, currentBytecode->current() - check_left);
	// if left is 0
	currentBytecode->addInsn(BC_POP);
	currentBytecode->addInsn(BC_POP);
	right->visit(this);
	convert_to_boolean(right);

	// set where to jump if left is 1
	currentBytecode->setInt16(set_result_if_left_is_true, currentBytecode->current() - set_result_if_left_is_true);
	break;

}

void CodeGenVisitor::convert_to_boolean(AstNode* node) {
	Bytecode* currentBytecode = getCurrentBytecode();
	if (*getNodeType(node) == VT_DOUBLE) {
		currentBytecode->addInsn(BC_DLOAD0);
		currentBytecode->addInsn(BC_DCMP);
	}
}


}


