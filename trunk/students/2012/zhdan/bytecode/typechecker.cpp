/*
 * typechecker.cpp
 *
 *  Created on: Oct 27, 2012
 *      Author: user
 */

#include "typechecker.h"
#include "astinfo.h"

namespace mathvm {

void TypeCheckerVisitor::visitBinaryOpNode(BinaryOpNode* node) {
	AstNode* left = node->left();
	AstNode* right = node->right();
	left->visit(this);
	right->visit(this);
	setNodeType(node, getOperationResultType(node->kind(), left, right));
}

void TypeCheckerVisitor::visitUnaryOpNode(UnaryOpNode* node) {
	AstNode* operand = node->operand();
	operand->visit(this);
	setNodeType(node, getOperationResultType(node->kind(), operand));
}

void TypeCheckerVisitor::visitBlockNode(BlockNode* node) {
	_current_scope = node->scope();
	int nodes_count = node->nodes();
	VarType* commonType = NULL;
	for (int i = 0; i < nodes_count; i++) {
		AstNode* currentNode = node->nodeAt(i);
		currentNode->visit(this);
		bool isBlockNode = currentNode->isForNode() || currentNode->isWhileNode() || currentNode->isIfNode();
		VarType* currentNodeType = getNodeType(currentNode);
		bool hasType = *currentNodeType != VT_VOID;
		if ((isBlockNode && hasType) || (i == nodes_count - 1)) {
			commonType = getUpperCommonType(commonType, currentNodeType);
		}
	}
	setNodeType(node, commonType);
	_current_scope = node->scope()->parent();
}

void TypeCheckerVisitor::visitCallNode(CallNode* node) {
	int params_count = node->parametersNumber();
	FunctionNode* refFunc = _current_scope->lookupFunction(node->name(), true);
	bool matchesRefedFunction = (refFunc!= NULL) && (refFunc->parametersNumber() != params_count);
	for (int i = 0; i < params_count; i++) {
		AstNode* param = node->parameterAt(i);
		param->visit(this);
		if (matchesRefedFunction && !isAssignable(&(refFunc->parameterType(i)), getNodeType(param))) {
			setErrorMessage(param, "Wrong type parameter");
		}
	}
	if (matchesRefedFunction) {
		setNodeType(node, &refFunc->returnType());
	} else {
		setNodeType(node, &VT_INVALID);
	}
}

void TypeCheckerVisitor::visitDoubleLiteralNode(DoubleLiteralNode* node) {
	setNodeType(node, &VT_DOUBLE);
}

void TypeCheckerVisitor::visitForNode(ForNode* node) {
	// todo set in expr type
	node->body()->visit(this);
	node->inExpr()->visit(this);
	setNodeType(node, getNodeType(node->body()));
}

void TypeCheckerVisitor::visitFunctionNode(FunctionNode* node) {
	node->body()->visit(this);
	if (!isAssignable(&node->returnType(), getNodeType(node->body()))) {
		setNodeType(node, &VT_INVALID);
	}
}

void TypeCheckerVisitor::visitIfNode(IfNode* node) {
	node->ifExpr()->visit(this);
	mathvm::BlockNode* thenBlock = node->thenBlock();
	thenBlock->visit(this);
	mathvm::BlockNode* elseBlock = node->elseBlock();
	elseBlock->visit(this);
	node->setInfo(new AstInfo(getUpperCommonType(getNodeType(thenBlock), getNodeType(elseBlock))));
}

void TypeCheckerVisitor::visitIntLiteralNode(IntLiteralNode* node) {
	setNodeType(node, &VT_INT);
}

void TypeCheckerVisitor::visitLoadNode(LoadNode* node) {
	setNodeType(node, &node->var()->type());
}

void TypeCheckerVisitor::visitNativeCallNode(NativeCallNode* node) {

}

void TypeCheckerVisitor::visitPrintNode(PrintNode* node) {
	node->visitChildren(this);
	setNodeType(node, &VT_VOID);
}

void TypeCheckerVisitor::visitReturnNode(ReturnNode* node) {
	node->returnExpr()->visit(this);
	setNodeType(node, getNodeType(node->returnExpr()));
}

void TypeCheckerVisitor::visitStoreNode(StoreNode* node) {
	AstNode* value = node->value();
	value->visit(this);
	AstVar* var = node->var();
	if (isAssignable(&var->type(), getNodeType(value))) {
		setNodeType(node, &var->type());
	} else {
		setErrorMessage(node, "Bad type");
	}
}

void TypeCheckerVisitor::visitStringLiteralNode(StringLiteralNode* node) {
	setNodeType(node, &VT_STRING);
}

void TypeCheckerVisitor::visitWhileNode(WhileNode* node) {
	node->whileExpr()->visit(this);
	if (*getNodeType(node->whileExpr()) == VT_VOID) {
		setErrorMessage(node, "Should be not void");
	}
	node->loopBlock()->visit(this);
	setNodeType(node, getNodeType(node->loopBlock()));
}

VarType* TypeCheckerVisitor::getOperationResultType(TokenKind tokenKind, AstNode* left, AstNode* right) {
	VarType* result = &VT_INVALID;
	VarType leftType = *getNodeType(left);
	VarType rightType = *getNodeType(right);
	switch (tokenKind) {
	case tOR: case tAND: case tEQ: case tNEQ:
	case tGT: case tGE: case tLT: case tLE:
		result = &VT_INT;
		break;
	case tADD:
		if (leftType == VT_STRING || rightType == VT_STRING) {
			result = &VT_STRING;
		} else if (leftType == VT_DOUBLE || rightType == VT_DOUBLE) {
			result = &VT_DOUBLE;
		} else if (leftType == VT_INT && rightType == VT_INT) {
			result = &VT_INT;
		}
		break;
	case tSUB: case tMUL: case tDIV: case tMOD:
		if (leftType == VT_DOUBLE || rightType == VT_DOUBLE) {
			result = &VT_DOUBLE;
		} else if (leftType == VT_INT && rightType == VT_INT) {
			result = &VT_INT;
		}
		break;
	case tASSIGN:
		if (!(isAssignable(&leftType, &rightType))) {
			setErrorMessage(right, "bad type");
		}
		result = &leftType;
		break;
	case tDECRSET:
		if (!(isAssignable(&leftType, &rightType)) || (leftType == VT_STRING)) {
			setErrorMessage(right, "bad type");
		}
		result = &leftType;
		break;
	case tINCRSET:
		if (leftType == VT_STRING) {
			result = &VT_STRING;
		} else if (leftType == VT_DOUBLE) {
			result = &VT_DOUBLE;
		} else if (leftType == VT_INT && rightType == VT_INT) {
			result = VT_INT;
		}
		break;
	}
	if (leftType == VT_VOID) {
		setErrorMessage(left, "Should not be of type void");
	}
	if (rightType == VT_VOID) {
		setErrorMessage(right, "Should not be of type void");
	}
	return result;
}

bool TypeCheckerVisitor::isAssignable(VarType* to, VarType* from) {
	if (*to == *from) {
		return true;
	}
	if (*to == VT_DOUBLE && *from == VT_INT) {
		return true;
	}
	return false;
}

VarType* TypeCheckerVisitor::getUpperCommonType(VarType* left, VarType* right) {
	if (*left == *right) {
		return left;
	}
	if (*left == VT_VOID || right == VT_VOID) {
		return &VT_VOID;
	}
	if (*left == VT_STRING || *right == VT_STRING) {
		return &VT_INVALID;
	}
	if (*left == VT_INVALID || right == VT_INVALID) {
		return &VT_INVALID;
	}
	if (*left == VT_INT && *right == VT_INT) {
		return &VT_INT;
	}
	return VT_DOUBLE;

}


}

