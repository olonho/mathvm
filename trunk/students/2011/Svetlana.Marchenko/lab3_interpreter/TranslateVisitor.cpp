#include "TranslateVisitor.h"
#include "Utility.h"

using namespace mathvm;

void TranslateVisitor::visitBlockNode( BlockNode* node) {
	bool needDeleteScope = false;
	if (_scope->functionScopeInitialized()) {
		ScopeWrapper *newScope = new ScopeWrapper(_scope, false, "", true);
		_scope = newScope;
		needDeleteScope = true;
	} else {
		_scope->setInitialized(true);
	}
	Scope* blockScope = node->scope();
	Scope::VarIterator varIt(blockScope);
	while (varIt.hasNext()) {
		AstVar* astVar = varIt.next();
		_scope->addVar(astVar->name());
	}
	Scope::FunctionIterator funcIt(blockScope);
	while (funcIt.hasNext()) {
		AstFunction* astFunc = funcIt.next();
		_code.addFunction(new  BytecodeFunction(astFunc));
	}
	node->visitChildren(this);
	funcIt = Scope::FunctionIterator(blockScope);
	while (funcIt.hasNext()) {
		AstFunction* astFunc = funcIt.next();
		astFunc->node()->visit(this);
	}
	if (needDeleteScope) {
		ScopeWrapper *parentScope = _scope->getParentScope();
		delete _scope;
		_scope = parentScope;
	}
}

void TranslateVisitor::visitIfNode( IfNode* node) {
	Label thenLabel(_byteCode);
	Label endLabel(_byteCode);

	node->ifExpr()->visit(this);
	_byteCode->addInsn(BC_ILOAD1);
	_byteCode->addBranch(BC_IFICMPE, thenLabel);

	if (node->elseBlock() != NULL) {
		node->elseBlock()->visit(this);
		_byteCode->addBranch(BC_JA, endLabel);
	} else {
		_byteCode->addBranch(BC_JA, endLabel);
	}
	_byteCode->bind(thenLabel);
	node->thenBlock()->visit(this);
	//_byteCode->addBranch(BC_JA, endLabel);
	_byteCode->bind(endLabel); 

}

void TranslateVisitor::visitWhileNode( WhileNode* node) {
	Label endLabel(_byteCode);
	Label loopLabel(_byteCode);

	_byteCode->bind(loopLabel);
	node->whileExpr()->visit(this);
	_byteCode->addInsn(BC_ILOAD0);
	_byteCode->addBranch(BC_IFICMPE, endLabel);
	node->loopBlock()->visit(this);
	_byteCode->addBranch(BC_JA, loopLabel);
	_byteCode->bind(endLabel);
}

void TranslateVisitor::visitLoadNode( LoadNode* node) {
	VarType type = node->var()->type();
	struct VarInfo varInfo = _scope->getVarIdByName(node->var()->name());
	bool theSameFunction = 
		(_scope->getFunctionInfo()._id == varInfo._functionId);
	_operandType = type;
	Instruction insn = BC_INVALID;
	switch (type) {
	case VT_INVALID:
		throw TranslationException("Type of variable is invalid");
		break;
	case VT_DOUBLE: 
		(theSameFunction) ? insn = BC_LOADDVAR : insn = BC_LOADCTXDVAR;
		break;
	case VT_INT:
		(theSameFunction) ? insn = BC_LOADIVAR : insn = BC_LOADCTXIVAR;
		break;
	case VT_STRING:
		(theSameFunction) ? insn = BC_LOADSVAR : insn = BC_LOADCTXSVAR;
		break;
	default:
		break;
	}
	_byteCode->addInsn(insn);
	if (!theSameFunction) {
		_byteCode->addUInt16(varInfo._functionId);
	} 
	_byteCode->addUInt16(varInfo._id);
}

void TranslateVisitor::visitStoreNode( StoreNode* node) {
	VarType varType = node->var()->type();
	TokenKind tokenKind = node->op();
	AstNode* valueNode = node->value();
	struct VarInfo varInfo = _scope->getVarIdByName(node->var()->name());
	bool theSameFunction = (_scope->getFunctionInfo()._id == varInfo._functionId);  	 
	Instruction insn = BC_INVALID; 

	switch (varType) {
	case VT_STRING:
		valueNode->visit(this);
		if ((_operandType == VT_STRING) && (tokenKind == tASSIGN)) {
			(theSameFunction) ? insn = BC_STORESVAR : insn = BC_STORECTXSVAR;
			_byteCode->addInsn(insn);
			if (!theSameFunction) _byteCode->addUInt16(varInfo._functionId);
			_byteCode->addUInt16(varInfo._id);
		} else {
			throw TranslationException("Couldn`t process any other operations with strings except assign");
		}
		break;
	case VT_INT:
		valueNode->visit(this);
		if (_operandType==VT_INT || _operandType==VT_DOUBLE) {
			if (valueNode->isDoubleLiteralNode())
				_byteCode->addInsn(BC_D2I);
			switch (tokenKind) {
			case tASSIGN:
				break;
			case tINCRSET:
				(theSameFunction) ? insn = BC_LOADIVAR : insn = BC_LOADCTXIVAR;
				_byteCode->addInsn(insn);
				if (!theSameFunction) _byteCode->addUInt16(varInfo._functionId);
				_byteCode->addUInt16(varInfo._id);
				_byteCode->addInsn(BC_IADD);
				break;
			case tDECRSET:
				(theSameFunction) ? insn = BC_LOADIVAR : insn = BC_LOADCTXIVAR;
				_byteCode->addInsn(insn);
				if (!theSameFunction) _byteCode->addUInt16(varInfo._functionId);
				_byteCode->addUInt16(varInfo._id);
				_byteCode->addInsn(BC_ISUB);
				break;
			default:
				break;
			}
			(theSameFunction) ? insn = BC_STOREIVAR : insn = BC_STORECTXIVAR;
			_byteCode->addInsn(insn);
			if (!theSameFunction) _byteCode->addUInt16(varInfo._functionId);
			_byteCode->addUInt16(varInfo._id);
		} else {
			throw TranslationException("Couldn`t store into int variable: Invalid operands type to store");
		}
		break;
	case VT_DOUBLE:
		valueNode->visit(this);
		if (_operandType == VT_DOUBLE || _operandType == VT_INT) {
			if (valueNode->isIntLiteralNode())
				_byteCode->addInsn(BC_I2D);
			switch (tokenKind) {
			case tASSIGN:
				break;
			case tINCRSET:
				(theSameFunction) ? insn = BC_LOADDVAR : insn = BC_LOADCTXDVAR;
				_byteCode->addInsn(insn);
				if (!theSameFunction) _byteCode->addUInt16(varInfo._functionId); 
				_byteCode->addUInt16(varInfo._id);
				_byteCode->addInsn(BC_DADD);
				break;
			case tDECRSET:
				(theSameFunction) ? insn = BC_LOADDVAR : insn = BC_LOADCTXDVAR;
				_byteCode->addInsn(insn);
				if (!theSameFunction) _byteCode->addUInt16(varInfo._functionId); 
				_byteCode->addUInt16(varInfo._id);
				_byteCode->addInsn(BC_DSUB);
				break;
			default:
				break;
			}
			(theSameFunction) ? insn = BC_STOREDVAR : insn = BC_STORECTXDVAR;
			_byteCode->addInsn(insn);
			if (!theSameFunction) _byteCode->addUInt16(varInfo._functionId);
			_byteCode->addUInt16(varInfo._id);
		}
		break;
	case VT_INVALID:
		throw TranslationException("Couldn`t store into double variable: Invalid operands type to store");
		break;
	default: break;
	}
}

void TranslateVisitor::visitUnaryOpNode( UnaryOpNode* node) {
	node->operand()->visit(this);
	TokenKind kind = node->kind();
	bool isInt = node->operand()->isIntLiteralNode();
	Label thenLabel, endLabel;
	//if (isInt || node->operand()->isDoubleLiteralNode()) {
		switch (kind) {
		case tSUB:
			node->visitChildren(this);
			isInt ? _byteCode->addInsn(BC_INEG): _byteCode->addInsn(BC_DNEG);
			break;
		case tNOT:
			//if (isInt) {
				thenLabel = Label(_byteCode);
				endLabel = Label(_byteCode);
				//node->visitChildren(this);
				_byteCode->addInsn(BC_ILOAD0);
				_byteCode->addBranch(BC_IFICMPE, thenLabel);
				_byteCode->addInsn(BC_ILOAD0);
				_byteCode->addBranch(BC_JA, endLabel);
				_byteCode->bind(thenLabel);
				_byteCode->addInsn(BC_ILOAD1);
				_byteCode->bind(endLabel);					
			//} else {
				//throw TranslationException("Operand type is invalid: could not apply logic not to not int variable");
			//}
			break;
		default:
			break;
		}
	//} else {
		//throw TranslationException("Operand type is invalid: could not apply unary operations to not int or double variable");
	//}
}

void TranslateVisitor::visitBinaryOpNode( BinaryOpNode* node) {
	TokenKind opKind = node->kind();
	VarType leftType;
	VarType rightType;
	Label returnTrueLabel(_byteCode);
	Label returnFalseLabel(_byteCode);
	Label endLabel(_byteCode);

	if (opKind == tADD || opKind == tSUB || opKind == tMUL || opKind == tDIV) {
		node->right()->visit(this);
		rightType = _operandType;
		node->left()->visit(this);
		leftType = _operandType;
		bool isInt = (leftType == VT_INT);
		if (rightType != leftType) {
			_byteCode->addInsn(BC_SWAP);
			(isInt) ? _byteCode->addInsn(BC_D2I) : _byteCode->addInsn(BC_I2D);
			_byteCode->addInsn(BC_SWAP);
		}
		switch (opKind) {
		case tADD:
			isInt ? _byteCode->addInsn(BC_IADD): _byteCode->addInsn(BC_DADD); break;
		case tSUB:
			isInt ? _byteCode->addInsn(BC_ISUB): _byteCode->addInsn(BC_DSUB); break;
		case tMUL:
			isInt ? _byteCode->addInsn(BC_IMUL): _byteCode->addInsn(BC_DMUL); break;
		case tDIV:
			isInt ? _byteCode->addInsn(BC_IDIV): _byteCode->addInsn(BC_DDIV); break;
		default:
			break;
		}
	
	} else if (opKind == tAND) {
		node->left()->visit(this);
		_byteCode->addInsn(BC_ILOAD1);
		_byteCode->addBranch(BC_IFICMPNE, returnFalseLabel);
		node->right()->visit(this);
		_byteCode->addInsn(BC_ILOAD1);
		_byteCode->addBranch(BC_IFICMPNE, returnFalseLabel);
		_byteCode->addInsn(BC_ILOAD1);
		_byteCode->addBranch(BC_JA, endLabel);
		_byteCode->bind(returnFalseLabel);
		_byteCode->addInsn(BC_ILOAD0);
		_byteCode->bind(endLabel);
	} else if (opKind == tOR) {
		node->left()->visit(this);
		_byteCode->addInsn(BC_ILOAD1);
		_byteCode->addBranch(BC_IFICMPE, returnTrueLabel);
		node->right()->visit(this);
		_byteCode->addInsn(BC_ILOAD1);
		_byteCode->addBranch(BC_IFICMPE, returnTrueLabel);
		_byteCode->addInsn(BC_ILOAD0);
		_byteCode->addBranch(BC_JA, endLabel);
		_byteCode->bind(returnTrueLabel);
		_byteCode->addInsn(BC_ILOAD1);
		_byteCode->bind(endLabel);
	} else {
		node->right()->visit(this);
		rightType = _operandType;
		node->left()->visit(this);
		leftType = _operandType;
		bool isInt = (leftType == VT_INT);
		if (leftType != rightType) {
			_byteCode->addInsn(BC_SWAP);
			(isInt) ? _byteCode->addInsn(BC_D2I) : _byteCode->addInsn(BC_I2D);
			_byteCode->addInsn(BC_SWAP);	
		}
		(isInt) ? _byteCode->addInsn(BC_ICMP) : _byteCode->addInsn(BC_DCMP);
		switch (opKind) {
			case tEQ:
				_byteCode->addInsn(BC_ILOAD0);
				_byteCode->addBranch(BC_IFICMPE, returnTrueLabel);
				break;
			case tNEQ:
				_byteCode->addInsn(BC_ILOAD0);
				_byteCode->addBranch(BC_IFICMPNE, returnTrueLabel);
				break;
			case tGT:
				_byteCode->addInsn(BC_ILOAD1);
				_byteCode->addBranch(BC_IFICMPE, returnTrueLabel);
				break;
			case tGE:
				_byteCode->addInsn(BC_ILOADM1);
				_byteCode->addBranch(BC_IFICMPNE, returnTrueLabel);
				break;
			case tLT:
				_byteCode->addInsn(BC_ILOADM1);
				_byteCode->addBranch(BC_IFICMPE, returnTrueLabel);
				break;
			case tLE:
				_byteCode->addInsn(BC_ILOAD1);
				_byteCode->addBranch(BC_IFICMPNE, returnTrueLabel);
				break;
			default:
				break;
		}
		_byteCode->addInsn(BC_ILOAD0);
		_byteCode->addBranch(BC_JA, endLabel);
		_byteCode->bind(returnTrueLabel);
		_byteCode->addInsn(BC_ILOAD1);
		_byteCode->bind(endLabel);
	}

}

void TranslateVisitor::visitDoubleLiteralNode( DoubleLiteralNode* node) {
	_operandType = VT_DOUBLE;
	_byteCode->addInsn(BC_DLOAD);
	_byteCode->addDouble(node->literal()); 

}

void TranslateVisitor::visitIntLiteralNode( IntLiteralNode* node) {
	_operandType = VT_INT;
	_byteCode->addInsn(BC_ILOAD);
	_byteCode->addInt64(node->literal());
}

void TranslateVisitor::visitStringLiteralNode( StringLiteralNode* node) {
	_operandType = VT_STRING;
	uint16_t strId = _code.makeStringConstant(node->literal());
	_byteCode->addInsn(BC_SLOAD);
	_byteCode->addUInt16(strId);
}

void TranslateVisitor::visitForNode( ForNode* node) {
	VarType varType = node->var()->type();
	if (varType != VT_INT)
		throw TranslationException("Invalid variable operand type for expression: only int type can be used");
	std::string varName = node->var()->name();
	struct VarInfo varInfo = _scope->getVarIdByName(varName);
	bool theSameFunction = (varInfo._functionId == _scope->getFunctionInfo()._id);
	Instruction insn = BC_INVALID;

	BinaryOpNode* inBinOp = dynamic_cast<BinaryOpNode*>(node->inExpr());
	if (!inBinOp || inBinOp->kind() != tRANGE) 
		throw TranslationException("Invalid binary operation for expression: only range operation can be used in for expression");

	inBinOp->left()->visit(this);
	if (_operandType != VT_INT)
		throw TranslationException("Invalid left operand type: only int type can be used for range operation");

	(theSameFunction) ? insn  = BC_STOREIVAR : insn = BC_STORECTXIVAR;
	_byteCode->addInsn(insn);
	if (!theSameFunction) _byteCode->addUInt16(varInfo._functionId);
	_byteCode->addUInt16(varInfo._id);

	Label startLoopLabel(_byteCode), endLabel(_byteCode);
	_byteCode->bind(startLoopLabel);
	inBinOp->right()->visit(this);
	if (_operandType != VT_INT) 
		throw TranslationException("Invalid right operand type: only int type can be used for range operation");
	(theSameFunction) ? insn  = BC_LOADIVAR : insn = BC_LOADCTXIVAR;
	_byteCode->addInsn(insn);
	if (!theSameFunction) _byteCode->addUInt16(varInfo._functionId);
	_byteCode->addUInt16(varInfo._id);

	_byteCode->addBranch(BC_IFICMPL, endLabel);
	node->body()->visit(this);

	(theSameFunction) ? insn  = BC_LOADIVAR : insn = BC_LOADCTXIVAR;
	_byteCode->addInsn(insn);
	if (!theSameFunction) _byteCode->addUInt16(varInfo._functionId);
	_byteCode->addUInt16(varInfo._id);
	_byteCode->addInsn(BC_ILOAD1);
	_byteCode->addInsn(BC_IADD);
	(theSameFunction) ? insn  = BC_STOREIVAR : insn = BC_STORECTXIVAR;
	_byteCode->addInsn(insn);
	if (!theSameFunction) _byteCode->addUInt16(varInfo._functionId);
	_byteCode->addUInt16(varInfo._id);
	_byteCode->addBranch(BC_JA, startLoopLabel);
	_byteCode->bind(endLabel);
}

void TranslateVisitor::visitPrintNode( PrintNode* node) {
	for(size_t i = 0; i < node->operands(); ++i) {
		AstNode* astNode = node->operandAt(i);
		astNode->visit(this);
		switch(_operandType) {
		case VT_STRING : _byteCode->addInsn(BC_SPRINT); break;
		case VT_INT : _byteCode->addInsn(BC_IPRINT); break;
		case VT_DOUBLE : _byteCode->addInsn(BC_DPRINT); break;
		default: break;
		}
	}
}

void TranslateVisitor::visitFunctionNode( FunctionNode* node) {
	//executes only for top function 
	if (!_code.functionByName(node->name())) { 
		BytecodeFunction *bcFunc = new  BytecodeFunction(extractAstFunction(node));
		bcFunc->setLocalsNumber(calculateFuncMaxLocals(node));
		_code.addFunction(bcFunc);
	}
	//save previous environment
	Bytecode *prevByteCode = _byteCode;
	_byteCode = new  Bytecode();
	ScopeWrapper *parentScope = _scope;
	_scope = new ScopeWrapper(parentScope, true, node->name(), false, &_code); 
	addParametersToScope(_scope, node);
	_operandType =  VT_INVALID;
	//process function body 
	node->body()->visit(this);
	if (prevByteCode == NULL) _byteCode->addInsn(BC_STOP);
	//add generated bytecode for the function into the corresponding TranslatedFunction from _code
	BytecodeFunction *bcFunc = static_cast< BytecodeFunction *>(_code.functionByName(node->name()));
	bcFunc->setLocalsNumber(calculateFuncMaxLocals(node));
	*(bcFunc->bytecode()) = *_byteCode;
	//free needless structures	
	delete _byteCode;
	_byteCode = prevByteCode;
	delete _scope;
	_scope = parentScope; 
}

void TranslateVisitor::addParametersToScope(ScopeWrapper* scope, FunctionNode* node) {
	AstFunction* astFunc = extractAstFunction(node);
	scope->setParametersNumber(astFunc->parametersNumber()); 
	for (uint16_t i = 0; i < astFunc->parametersNumber(); ++i) {
		scope->addVar(astFunc->parameterName(i));	
	}
}

void TranslateVisitor::visit(  BlockNode * rootNode )
{
	rootNode->visit(this);
}

void TranslateVisitor::visitCallNode(CallNode* node) {
	for (uint16_t i =0; i < node->parametersNumber(); ++i) {
		node->parameterAt(i)->visit(this);
	}
	_byteCode->addInsn(BC_CALL);
	TranslatedFunction *callFunction = _code.functionByName(node->name());
	_byteCode->addUInt16(callFunction->id());
}

void TranslateVisitor::visitReturnNode(ReturnNode* node) {
	if (node->returnExpr()) node->returnExpr()->visit(this);
	_byteCode->addInsn(BC_RETURN);
}

void TranslateVisitor::visitNativeCallNode(NativeCallNode* node) {

}


Code* TranslateVisitor::getBytecode()
{
	return &_code;
}

std::vector<std::string> TranslateVisitor::getStringsVector()
{
	vector<string> result;
	for (uint16_t i = 0; i < 256; ++i) {
		string s = _code.constantById(i);
		result.push_back(s);
	}
	return result;
}

void TranslateVisitor::dump()
{
	_code.disassemble();
}
