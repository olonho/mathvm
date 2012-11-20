/*
 * BytecodeVisitor.cpp
 *
 *  Created on: 24.10.2012
 *      Author: temp1ar
 */

#include "BytecodeVisitor.h"

#define PLACE_HOLDER 0

namespace mathvm {

BytecodeVisitor::BytecodeVisitor(AstFunction* top, Code* code)
	: top_(top)
	, code_(code)
{}

BytecodeVisitor::~BytecodeVisitor() {
}

void BytecodeVisitor::compare() {
	uint32_t placeHolder1;
	uint32_t placeHolder2;
	uint32_t placeHolder3;
	placeHolder1 = bc()->current();
	bc()->addInt16(PLACE_HOLDER);
	bc()->addInsn(BC_JA);
	placeHolder2 = bc()->current();
	bc()->addInt16(PLACE_HOLDER);
	bc()->setInt16(placeHolder1, bc()->current() - placeHolder1);
	bc()->addInsn(BC_POP);
	bc()->addInsn(BC_POP);
	bc()->addInsn(BC_ILOAD1);
	bc()->addInsn(BC_JA);
	placeHolder3 = bc()->current();
	bc()->addInt16(PLACE_HOLDER);
	bc()->setInt16(placeHolder2, bc()->current() - placeHolder2);
	bc()->addInsn(BC_POP);
	bc()->addInsn(BC_POP);
	bc()->addInsn(BC_ILOAD0);
	bc()->setInt16(placeHolder3, bc()->current() - placeHolder3);
}

void BytecodeVisitor::visitBinaryOpNode(BinaryOpNode* node) {
	node->right()->visit(this);
	node->left()->visit(this);

	switch(node->kind()) {
		case tADD:
			bc()->addInsn(BC_IADD);
			break;
		case tSUB:
			bc()->addInsn(BC_SWAP);
			bc()->addInsn(BC_ISUB);
			break;
		case tMUL:
			bc()->addInsn(BC_IMUL);
			break;
		case tDIV:
			bc()->addInsn(BC_SWAP);
			bc()->addInsn(BC_IDIV);
			break;
		case tMOD:
			bc()->addInsn(BC_SWAP);
			bc()->addInsn(BC_IMOD);
			break;
		case tOR: case tAND:
			bc()->addInsn(BC_STOREIVAR0);
			bc()->addInsn(BC_ILOAD0);
			bc()->addInsn(BC_IFICMPNE);
			compare();
			bc()->addInsn(BC_LOADIVAR0);
			bc()->addInsn(BC_ILOAD0);
			bc()->addInsn(BC_IFICMPNE);
			compare();
			break;
		case tEQ:
			bc()->addInsn(BC_IFICMPE);
			break;
		case tNEQ:
			bc()->addInsn(BC_IFICMPNE);
			break;
		case tGT:
			bc()->addInsn(BC_IFICMPG);
			break;
		case tGE:
			bc()->addInsn(BC_IFICMPGE);
			break;
		case tLT:
			bc()->addInsn(BC_IFICMPL);
			break;
		case tLE:
			bc()->addInsn(BC_IFICMPLE);
			break;
		default:
			bc()->addInsn(BC_INVALID);
			break;
	}

	switch(node->kind()) {
		case tEQ: case tNEQ:
		case tGT: case tGE:
		case tLT: case tLE:
			compare();
		break;

		case tOR:
			bc()->addInsn(BC_IADD);
			bc()->addInsn(BC_ILOAD);
			bc()->addInt64(0);
			bc()->addInsn(BC_IFICMPL);
			compare();
			break;

		case tAND:
			bc()->addInsn(BC_IMUL);
			break;

		default:
			break;
	}
}

void BytecodeVisitor::visitUnaryOpNode(UnaryOpNode* node) {
	node->operand()->visit(this);
	switch(node->kind()) {
	case tNOT:
		bc()->addInsn(BC_ILOAD0);
		bc()->addInsn(BC_IFICMPNE);
		compare();
		break;
	case tSUB:
		bc()->addInsn(BC_INEG);
		break;
	default:
		break;
	}
}

void BytecodeVisitor::visitStringLiteralNode(StringLiteralNode* node) {
}

void BytecodeVisitor::visitDoubleLiteralNode(DoubleLiteralNode* node) {
}

void BytecodeVisitor::visitIntLiteralNode(IntLiteralNode* node) {
	bc()->addInsn(BC_ILOAD);
	bc()->addInt64(node->literal());
}

void BytecodeVisitor::visitLoadNode(LoadNode* node) {
	bc()->addInsn(BC_LOADIVAR);
	bc()->addUInt16(vars_[node->var()]);
}

void BytecodeVisitor::visitStoreNode(StoreNode* node) {
	node->visitChildren(this);
	bc()->addInsn(BC_STOREIVAR);
	bc()->addUInt16(vars_[node->var()]);
}

void BytecodeVisitor::visitForNode(ForNode* node) {
}

void BytecodeVisitor::visitWhileNode(WhileNode* node) {
	uint32_t placeHolder1;
	uint32_t placeHolder2;
	uint32_t placeHolder3;
	placeHolder1 = bc()->current();
	node->whileExpr()->visit(this);
	bc()->addInsn(BC_ILOAD0);
	bc()->addInsn(BC_IFICMPE);
	placeHolder2 = bc()->current();
	bc()->addInt16(PLACE_HOLDER);
	bc()->addInsn(BC_JA);
	placeHolder3 = bc()->current();
	bc()->addInt16(PLACE_HOLDER);
	bc()->setInt16(placeHolder2, bc()->current() - placeHolder2);
	bc()->addInsn(BC_POP);
	bc()->addInsn(BC_POP);
	node->loopBlock()->visit(this);
	bc()->addInsn(BC_JA);
	bc()->addInt16(placeHolder1 - bc()->current());
	bc()->setInt16(placeHolder3, bc()->current() - placeHolder3);
	bc()->addInsn(BC_POP);
	bc()->addInsn(BC_POP);
}

void BytecodeVisitor::visitIfNode(IfNode* node) {
	node->ifExpr()->visit(this);
	bc()->addInsn(BC_ILOAD0);
	bc()->addInsn(BC_IFICMPNE);
	uint32_t placeHolder1;
	uint32_t placeHolder2;
	uint32_t placeHolder3;
	placeHolder1 = bc()->current();
	bc()->addInt16(PLACE_HOLDER);
	bc()->addInsn(BC_JA);
	placeHolder2 = bc()->current();
	bc()->addInt16(PLACE_HOLDER);
	bc()->setInt16(placeHolder1, bc()->current() - placeHolder1);
	bc()->addInsn(BC_POP);
	bc()->addInsn(BC_POP);
	node->thenBlock()->visit(this);
	bc()->addInsn(BC_JA);
	placeHolder3 = bc()->current();
	bc()->addInt16(PLACE_HOLDER);
	bc()->setInt16(placeHolder2, bc()->current() - placeHolder2);
	bc()->addInsn(BC_POP);
	bc()->addInsn(BC_POP);
	if (node->elseBlock()) {
		node->elseBlock()->visit(this);
	}
	bc()->setInt16(placeHolder3, bc()->current() - placeHolder3);
}

void BytecodeVisitor::visitBlockNode(BlockNode* node) {
	scopesStack_.push(node->scope());
	blockContents(node);
	scopesStack_.pop();
}

void BytecodeVisitor::visitFunctionNode(FunctionNode* node) {
	node->body()->visit(this);
}

void BytecodeVisitor::visitReturnNode(ReturnNode* node) {
	if (node->returnExpr() != 0) {
		node->returnExpr()->visit(this);
	}
	bc()->addInsn(BC_RETURN);
}

void BytecodeVisitor::visitCallNode(CallNode* node) {
    for(size_t i = 0; i < node->parametersNumber(); ++i) {
        node->parameterAt(i)->visit(this);
    }
    bc()->addInsn(BC_CALL);
    bc()->addInt16(functions_[scopesStack_.top()->lookupFunction(node->name(), true)]->id());
}

void BytecodeVisitor::visitNativeCallNode(NativeCallNode* node) {
}

void BytecodeVisitor::visitPrintNode(PrintNode* node) {
	for (size_t i = 0; i != node->operands(); ++i)
	{
		node->operandAt(i)->visit(this);
		bc()->addInsn(BC_IPRINT);
	}
}

void BytecodeVisitor::visit() {
	functionDeclarations(top_->scope());
}

void BytecodeVisitor::functionDeclarations(Scope* scope)
{
    Scope::FunctionIterator fNative(scope);
    vector<AstFunction*> delayedDeclarations;
    while(fNative.hasNext()) {
        AstFunction* func = fNative.next();
        if (func->node()->body()->nodeAt(0)->isNativeCallNode())
            func->node()->visit(this);
        else
            delayedDeclarations.push_back(func);
    }

    for(size_t i = 0; i < delayedDeclarations.size(); ++i) {
    	BytecodeFunction* function = new BytecodeFunction(delayedDeclarations[i]);
    	code_->addFunction(function);
    	functions_.insert(make_pair(delayedDeclarations[i], function));
    	functionsStack_.push(function);
        delayedDeclarations[i]->node()->visit(this);
        functionsStack_.pop();
    }
}

void BytecodeVisitor::variableDeclarations( Scope* scope )
{
	static uint16_t varId = 0;
	Scope::VarIterator vIter(scope);
	while(vIter.hasNext()) {
        AstVar* variable = vIter.next();
        vars_[variable] = varId++;
    }
}

void BytecodeVisitor::blockContents(BlockNode* node)
{
    // Functions and variables
    variableDeclarations(node->scope());
    functionDeclarations(node->scope());

    // Code part
	for(size_t i = 0; i < node->nodes(); ++i) {
		node->nodeAt(i)->visit(this);
	}
}

} /* namespace mathvm */

