/*
 * BytecodeVisitor.cpp
 *
 *  Created on: 24.10.2012
 *      Author: temp1ar
 */

#include "BytecodeVisitor.h"

namespace mathvm {

BytecodeVisitor::BytecodeVisitor(AstFunction* top, Code* code)
	: top_(top)
	, code_(code)
    , varId(0)
    , typeOfTOS(VT_INVALID)
{}

BytecodeVisitor::~BytecodeVisitor() {
}

void BytecodeVisitor::toBoolean() {
    bc()->addInsn(BC_ILOAD0);
    compare(BC_IFICMPNE);
}

void BytecodeVisitor::compare(Instruction kind) {
    Label elseLabel(bc());
    Label endIf(bc());
    bc()->addBranch(kind, elseLabel);
    bc()->addInsn(BC_POP);
    bc()->addInsn(BC_POP);
    bc()->addInsn(BC_ILOAD0);
    bc()->addBranch(BC_JA, endIf);
    elseLabel.bind(bc()->current(), bc());
    bc()->addInsn(BC_POP);
    bc()->addInsn(BC_POP);
    bc()->addInsn(BC_ILOAD1);
    endIf.bind(bc()->current(), bc());	
}

void BytecodeVisitor::visitBinaryOpNode(BinaryOpNode* node) {
	node->right()->visit(this);
	node->left()->visit(this);

	switch(node->kind()) {
		case tADD:
            switch (typeOfTOS)
            {
            case VT_INT:
                bc()->addInsn(BC_IADD);
                break;
            case VT_DOUBLE:
                bc()->addInsn(BC_DADD);
                break;
            default:
                //assert(false);
                break;
            }
			break;
		case tSUB:
            switch (typeOfTOS)
            {
            case VT_INT:
                bc()->addInsn(BC_ISUB);
                break;
            case VT_DOUBLE:
                bc()->addInsn(BC_DSUB);
                break;
            default:
                assert(false);
                break;
            }
			break;
		case tMUL:
            switch (typeOfTOS)
            {
            case VT_INT:
                bc()->addInsn(BC_IMUL);
                break;
            case VT_DOUBLE:
                bc()->addInsn(BC_DMUL);
                break;
            default:
                assert(false);
                break;
            }
			break;
		case tDIV:
            switch (typeOfTOS)
            {
            case VT_INT:
                bc()->addInsn(BC_IDIV);
                break;
            case VT_DOUBLE:
                bc()->addInsn(BC_DDIV);
                break;
            default:
                assert(false);
                break;
            }
			break;
		case tMOD:
			bc()->addInsn(BC_IMOD);
			break;
		case tOR: case tAND:
			toBoolean();
			bc()->addInsn(BC_SWAP);
			toBoolean();
			bc()->addInsn(BC_SWAP);
            break;
		case tEQ:
			compare(BC_IFICMPE);
			break;
		case tNEQ:
			compare(BC_IFICMPNE);
			break;
		case tGT:
			compare(BC_IFICMPG);
			break;
		case tGE:
			compare(BC_IFICMPGE);
			break;
		case tLT:
			compare(BC_IFICMPL);
			break;
		case tLE:
			compare(BC_IFICMPLE);
			break;
        case tRANGE:
            break;
		default:
			bc()->addInsn(BC_INVALID);
			break;
	}

	switch(node->kind()) {
		case tOR:
			bc()->addInsn(BC_IADD);
			toBoolean();
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
		compare(BC_IFICMPE);
		break;
	case tSUB:
        switch (typeOfTOS)
        {
        case VT_INT:
            bc()->addInsn(BC_INEG);
            break;
        case VT_DOUBLE:
            bc()->addInsn(BC_DNEG);
            break;
        default:
            assert(false);
            break;
        }
		break;
	default:
		break;
	}
}

void BytecodeVisitor::visitStringLiteralNode(StringLiteralNode* node) {
    bc()->addInsn(BC_SLOAD);
    bc()->addUInt16(code_->makeStringConstant(node->literal()));
    typeOfTOS = VT_STRING;
}

void BytecodeVisitor::visitDoubleLiteralNode(DoubleLiteralNode* node) {
    bc()->addInsn(BC_DLOAD);
    bc()->addDouble(node->literal());
    typeOfTOS = VT_DOUBLE;
}

void BytecodeVisitor::visitIntLiteralNode(IntLiteralNode* node) {
	bc()->addInsn(BC_ILOAD);
	bc()->addInt64(node->literal());
    typeOfTOS = VT_INT;
}

void BytecodeVisitor::visitLoadNode(LoadNode* node) {
    switch (node->var()->type())
    {
    case VT_INT:
        bc()->addInsn(BC_LOADIVAR);
        break;
    case VT_DOUBLE:
        bc()->addInsn(BC_LOADDVAR);
        break;
    case VT_STRING:
        bc()->addInsn(BC_LOADSVAR);
        break;
    case VT_INVALID:
        break;
    default:
        assert(false);
        break;
    }
	bc()->addUInt16(vars_[node->var()].second);
    typeOfTOS = node->var()->type();
}

void BytecodeVisitor::store(uint16_t functionId, uint16_t id, VarType type) {
    switch (type)
    {
    case VT_INT:
        bc()->addInsn(BC_STOREIVAR);
        break;
    case VT_DOUBLE:
        bc()->addInsn(BC_STOREDVAR);
        break;
    case VT_STRING:
        bc()->addInsn(BC_STORESVAR);
        break;
    default:
        assert(false);
        break;
    }
    bc()->addUInt16(id);
}

void BytecodeVisitor::visitStoreNode(StoreNode* node) {
	node->visitChildren(this);
    switch (node->op())
    {
    case tASSIGN:
        store(vars_[node->var()].first, vars_[node->var()].second, node->var()->type());
        break;
    case tINCRSET:
        switch (typeOfTOS)
        {
        case VT_INT:
            bc()->addInsn(BC_LOADIVAR);
            bc()->addUInt16(vars_[node->var()].second);
            bc()->addInsn(BC_IADD);
            store(vars_[node->var()].first, vars_[node->var()].second, node->var()->type());
            break;
        case VT_DOUBLE:
            bc()->addInsn(BC_LOADDVAR);
            bc()->addUInt16(vars_[node->var()].second);
            bc()->addInsn(BC_DADD);
            store(vars_[node->var()].first, vars_[node->var()].second, node->var()->type());
            break;
        default:
            assert(false);
            break;
        }
        break;
    case tDECRSET:
        switch (typeOfTOS)
        {
        case VT_INT:
            bc()->addInsn(BC_LOADIVAR);
            bc()->addUInt16(vars_[node->var()].second);
            bc()->addInsn(BC_ISUB);
            store(vars_[node->var()].first, vars_[node->var()].second, node->var()->type());
            break;
        case VT_DOUBLE:
            bc()->addInsn(BC_LOADDVAR);
            bc()->addUInt16(vars_[node->var()].second);
            bc()->addInsn(BC_DSUB);
            store(vars_[node->var()].first, vars_[node->var()].second, node->var()->type());
            break;
        default:
            assert(false);
            break;
        }
        break;
    default:
        break;
    }
}

void BytecodeVisitor::visitForNode(ForNode* node) {
    BinaryOpNode* range = node->inExpr()->asBinaryOpNode();
    range->visit(this);
    Label conditionLabel(bc(), bc()->current());
    store(vars_[node->var()].first, vars_[node->var()].second, node->var()->type());
    node->body()->visit(this);
    bc()->addInsn(BC_LOADIVAR);
    bc()->addInt16(vars_[node->var()].second);
    bc()->addInsn(BC_ILOAD1);
    bc()->addInsn(BC_IADD);
    bc()->addBranch(BC_IFICMPLE, conditionLabel);
}

void BytecodeVisitor::visitWhileNode(WhileNode* node) {
	Label conditionLabel(bc(), bc()->current());
    Label endWhile(bc());
    node->whileExpr()->visit(this);
	bc()->addInsn(BC_ILOAD0);
    bc()->addBranch(BC_IFICMPE, endWhile);
	bc()->addInsn(BC_POP);
	bc()->addInsn(BC_POP);
	node->loopBlock()->visit(this);
	bc()->addBranch(BC_JA, conditionLabel);
    endWhile.bind(bc()->current(), bc());
	bc()->addInsn(BC_POP);
	bc()->addInsn(BC_POP);
}

void BytecodeVisitor::visitIfNode(IfNode* node) {
    node->ifExpr()->visit(this);
    bc()->addInsn(BC_ILOAD0);
    Label elseLabel(bc());
    Label endIf(bc());
    bc()->addBranch(BC_IFICMPE, elseLabel);
	bc()->addInsn(BC_POP);
	bc()->addInsn(BC_POP);
	node->thenBlock()->visit(this);
	bc()->addBranch(BC_JA, endIf);
    elseLabel.bind(bc()->current(), bc());
	bc()->addInsn(BC_POP);
	bc()->addInsn(BC_POP);
	if (node->elseBlock()) {
		node->elseBlock()->visit(this);
	}
    endIf.bind(bc()->current(), bc());
}

void BytecodeVisitor::visitBlockNode(BlockNode* node) {
	scopesStack_.push(node->scope());
	blockContents(node);
	scopesStack_.pop();
}

void BytecodeVisitor::visitFunctionNode(FunctionNode* node) {
    varId = 0;
    for(size_t i = 0; i < node->parametersNumber(); ++i) {
        AstVar* variable = node->body()->scope()->lookupVariable(node->parameterName(i));
        pair<uint16_t, uint16_t> key = make_pair(functionsStack_.top()->id(), varId++);
        vars_[variable] = key;
        store(functionsStack_.top()->id(), i, variable->type());
    }
	node->body()->visit(this);
}

void BytecodeVisitor::visitReturnNode(ReturnNode* node) {
	if (node->returnExpr() != 0) {
		node->returnExpr()->visit(this);
	}
	bc()->addInsn(BC_RETURN);
}

void BytecodeVisitor::visitCallNode(CallNode* node) {
    BytecodeFunction* function = functions_[scopesStack_.top()->lookupFunction(node->name(), true)];
    for(size_t i = 0; i < node->parametersNumber(); ++i) {
        node->parameterAt(node->parametersNumber() - 1 - i)->visit(this);        
    }
    bc()->addInsn(BC_CALL);
    bc()->addInt16(function->id());
    typeOfTOS = function->returnType();
}

void BytecodeVisitor::visitNativeCallNode(NativeCallNode* node) {
}

void BytecodeVisitor::visitPrintNode(PrintNode* node) {
	for (size_t i = 0; i != node->operands(); ++i)
	{
		node->operandAt(i)->visit(this);
        switch (typeOfTOS)
        {
        case VT_INT:
            bc()->addInsn(BC_IPRINT);
            break;
        case VT_DOUBLE:
            bc()->addInsn(BC_DPRINT);
            break;
        case VT_STRING:
            bc()->addInsn(BC_SPRINT);
            break;
        default:
            assert(false);
            break;
        }
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
	Scope::VarIterator vIter(scope);
	while(vIter.hasNext()) {
        AstVar* variable = vIter.next();
        vars_[variable] = make_pair(functionsStack_.top()->id(), varId++);
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

