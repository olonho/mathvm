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
{
	XMMReg xmmRegisters[] = {xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7};
	xmmRegisters_.assign(xmmRegisters, xmmRegisters + 8);

	GPReg gpRegisters[] = {rdi, rsi, rdx, rcx, r8, r9};
	gpRegisters_.assign(gpRegisters, gpRegisters + 6);
}

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
    load(node->var());
    typeOfTOS = node->var()->type();
}

void BytecodeVisitor::store(uint16_t functionId, uint16_t id, VarType type) {
    bool local = (functionId == functionsStack_.top()->id());
    
    switch (type)
    {
    case VT_INT:
        bc()->addInsn(local ? BC_STOREIVAR : BC_STORECTXIVAR);
        break;
    case VT_DOUBLE:
        bc()->addInsn(local ? BC_STOREDVAR : BC_STORECTXDVAR);
        break;
    case VT_STRING:
        bc()->addInsn(local ? BC_STORESVAR : BC_STORECTXSVAR);
        break;
    default:
        assert(false);
        break;
    }
    if (!local)
        bc()->addUInt16(functionId);
    bc()->addUInt16(id);
}

void BytecodeVisitor::store(const AstVar* var) {
	store(vars_[var].first, vars_[var].second, var->type());
}

void BytecodeVisitor::load(uint16_t functionId, uint16_t id, VarType type) {
    bool local = (functionId == functionsStack_.top()->id());

    switch (type)
    {
    case VT_INT:
        bc()->addInsn(local ? BC_LOADIVAR : BC_LOADCTXIVAR);
        break;                              
    case VT_DOUBLE:                         
        bc()->addInsn(local ? BC_LOADDVAR : BC_LOADCTXDVAR);
        break;                              
    case VT_STRING:                         
        bc()->addInsn(local ? BC_LOADSVAR : BC_LOADCTXSVAR);
        break;
    default:
        assert(false);
        break;
    }
    if (!local)
        bc()->addUInt16(functionId);
    bc()->addUInt16(id);
}

void BytecodeVisitor::load(const AstVar* var) {
	load(vars_[var].first, vars_[var].second, var->type());
}


void BytecodeVisitor::visitStoreNode(StoreNode* node) {
	node->visitChildren(this);
    switch (node->op())
    {
    case tASSIGN:
        store(node->var());
        break;
    case tINCRSET:
        switch (typeOfTOS)
        {
        case VT_INT:
        	load(node->var());
            bc()->addInsn(BC_IADD);
            store(node->var());
            break;
        case VT_DOUBLE:
        	load(node->var());
            bc()->addInsn(BC_DADD);
            store(node->var());
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
        	load(node->var());
            bc()->addInsn(BC_ISUB);
            store(node->var());
            break;
        case VT_DOUBLE:
        	load(node->var());
            bc()->addInsn(BC_DSUB);
            store(node->var());
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
    store(node->var());
    node->body()->visit(this);
    load(node->var());
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
		Assembler _;
	    //FileLogger logger(stdout);
	    //_.setLogger(&logger);

		// Callee prologue
	    _.push(rbp);
	    // Save caller's stack pointer
	    _.mov(rbp, rsp);

	    // If there are arguments
	    if (node->nativeSignature().size() > 1)
	    {
	    	// rdi contains address of our buffer
	        _.mov(r11, rdi);

	        size_t gpCounter = 0;
	        size_t xmmCounter = 0;
	        size_t offset = 0;
	        vector<Mem> miniStack;
	        int ptrSize = sizeof(char *);
	        for (size_t i = 1; i < node->nativeSignature().size(); ++i)
	        {
	        	switch(node->nativeSignature()[i].first) {
	        	case VT_INT:
	        		// If there are more than 6 gp variables - put all other on stack
	        		if (gpCounter < 6) {
	        			_.mov(gpRegisters_[gpCounter++], qword_ptr(r11, offset));
	        		} else {
	        			miniStack.push_back(qword_ptr(r11, offset));
	        		}
	        		break;

	        	case VT_DOUBLE:
	        		// For floating point arguments XMM0-XMM7 are used
	        		if (xmmCounter < 8) {
		                _.movsd(xmmRegisters_[xmmCounter++], qword_ptr(r11, offset));
	        		} else {
	        			miniStack.push_back(qword_ptr(r11, offset));
	        		}
	        		break;

	        	case VT_STRING:
	        		if (gpCounter < 6) {
	        			_.mov(gpRegisters_[gpCounter++], qword_ptr(r11, offset));
	        		} else {
	        			miniStack.push_back(qword_ptr(r11, offset));
	        		}
	        		break;
	        	default:
	        		assert(false);
	        		break;
	        	}

                offset += ptrSize;
	        }

	        vector<Mem>::reverse_iterator it = miniStack.rbegin();
	        for(; it != miniStack.rend(); ++it) {
	        	_.push(*it);
	        }
	    }

	    void *native = dlsym(RTLD_DEFAULT, node->nativeName().c_str());
	    _.call(imm((sysint_t)native));

	    // Callee epilogue, return caller's stack pointer
	    _.mov(rsp, rbp);
	    _.pop(rbp);
	    _.ret();

		uint16_t id = code_->makeNativeFunction(node->nativeName(), node->nativeSignature(), _.make());
		bc()->addInsn(BC_CALLNATIVE);
		bc()->addInt16(id);
		bc()->addInsn(BC_RETURN);
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
    Scope::FunctionIterator fIterator(scope);

    vector<pair<AstFunction*, BytecodeFunction*> > toVisit;

    // Declarations & native visiting
    while(fIterator.hasNext()) {
        AstFunction* astFunction = fIterator.next();
    	BytecodeFunction* bcFunction = new BytecodeFunction(astFunction);
    	code_->addFunction(bcFunction);
    	functions_.insert(make_pair(astFunction, bcFunction));
    	if (astFunction->node()->body()->nodeAt(0)->isNativeCallNode()) {
        	functionsStack_.push(bcFunction);
    		astFunction->node()->body()->nodeAt(0)->visit(this);
            functionsStack_.pop();
    	} else {
    		toVisit.push_back(make_pair(astFunction, bcFunction));
    	}
    }

    // All other visiting
    for(size_t i = 0; i < toVisit.size(); ++i) {
    	functionsStack_.push(toVisit[i].second);
    	toVisit[i].first->node()->visit(this);
        functionsStack_.pop();
    }
}

void BytecodeVisitor::variableDeclarations( Scope* scope )
{
	Scope::VarIterator vIter(scope);
	while(vIter.hasNext()) {
        AstVar* variable = vIter.next();
        vars_[variable] = make_pair(functionsStack_.top()->id(), varId++);
        functionsStack_.top()->setLocalsNumber(functionsStack_.top()->localsNumber() + 1);
    }
    //
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

