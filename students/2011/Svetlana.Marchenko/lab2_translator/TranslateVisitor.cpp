 #include "TranslateVisitor.h"
 #include "Utility.h"
 
 using namespace mathvm;
 
 void TranslateVisitor::visitBlockNode(mathvm::BlockNode* node) {
	 bool needDeleteScope = false;
	 if (_scope->functionScopeInitialized()) {
		Scope *newScope = new Scope(_scope, false, "", true);
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
		_code.addFunction(new mathvm::BytecodeFunction(astFunc));
	 }
	 node->visitChildren(this);
	 funcIt = mathvm::FunctionIterator(blockScope);
	 while (funcIt.hasNext()) {
		AstFunction* astFunc = funcIt.next();
		astFunc->node()->visit(this);
	 }
	 if (needDeleteScope) {
		Scope *parentScope = _scope->getParentScope();
		delete _scope;
		_scope = parentScope;
	 }
 }
 
 void TranslateVisitor::visitIfNode(mathvm::IfNode* node) {
	 Label thenLabel(&_byteCode);
	 Label endLabel(&_byteCode);
	 
	 node->ifExpr()->visit(this);
	 _byteCode->addInsn(BC_ILOAD1);
	 _byteCode->addBranch(BC_IFICMPE, thenLabel);
	 
	 if (node->elseBlock() != NULL) {
		 node->elseBlock()->visit(this);
		 _byteCode->addBranch(BC_JA, endLabel);
	 }
	 _byteCode->bind(thenLabel);
	 node->thenBlock()->visit(this);
	 _byteCode->addBranch(BC_JA, endLabel);
	 _byteCode->bind(endLabel); 
	 
 }
 
 void TranslateVisitor::visitWhileNode(mathvm::WhileNode* node) {
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
 
  void TranslateVisitor::visitLoadNode(mathvm::LoadNode* node) {
	  VarType type = node->var()->type();
	  struct VarInfo varInfo = _scope->getVarIdByName(node->var()->name());
	  bool theSameFunction = 
		(_scope->getFunctionInfo()._id == varInfo._functionId);
      _operandType = type;
      mathvm::Instruction insn = BC_INVALID;
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
			_byteCode->addUint16(varInfo._functionId);
	  } 
	  _byteCode->addUInt16(varInfo._id);
  }
  
  void TranslateVisitor::visitStoreNode(mathvm::StoreNode* node) {
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
				_byteCode.addInsn(insn);
				if (!theSameFunction) _byteCode->addUint16(varInfo._functionId);
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
						if (!theSameFunction) _byteCode->addUint16(varInfo._functionId);
						_byteCode->addUInt16(varInfo._id);
						_byteCode->addInsn(BC_IADD);
						break;
					case tDECRSET:
						(theSameFunction) ? insn = BC_LOADIVAR : insn = BC_LOADCTXIVAR;
						_byteCode.addInsn(insn);
						if (!theSameFunction) _byteCode->addUint16(varInfo._functionId);
						_byteCode->addUInt16(varInfo._id);
						_byteCode->addInsn(BC_SWAP);
						_byteCode->addInsn(BC_ISUB);
						break;
					default:
						break;
				}
				(theSameFunction) ? insn = BC_STOREIVAR : insn = BC_STORECTXIVAR;
				_byteCode.addInsn(insn);
				if (!theSameFunction) _byteCode->addUint16(varInfo._functionId);
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
 
  void TranslateVisitor::visitUnaryOpNode(mathvm::UnaryOpNode* node) {
	  TokenKind kind = node->kind();
	  bool isInt = node->operand()->isIntLiteralNode();
	  if (isInt || node->operand()->isDoubleLiteralNode()) {
		  switch (kind) {
			  case tSUB:
				node->visitChildren(this);
				isInt ? _byteCode->addInsn(BC_INEG): _byteCode->addInsn(BC_DNEG);
				break;
			  case tNOT:
				if (isInt) {
					Label thenLabel(&_byteCode);
					Label endLabel(&_byteCode);
					node->visitChildren(this);
					_byteCode->addInsn(BC_ILOAD0);
					_byteCode->addBranch(BC_IFICMPE, thenLabel);
					_byteCode->addInsn(BC_ILOAD1);
					_byteCode->addBranch(BC_JA, endLabel);
					_byteCode->bind(thenLabel);
					_byteCode->addInsn(BC_ILOAD0);
					_byteCode->bind(endLabel);					
				} else {
					throw TranslationException("Operand type is invalid: could not apply logic not to not int variable");
				}
				break;
			default:
				break;
		  }
	  } else {
		  throw TranslationException("Operand type is invalid: could not apply unary operations to not int or double variable");
	  }
  }
 
 void TranslateVisitor::visitBinaryOpNode(mathvm::BinaryOpNode* node) {
	 TokenKind opKind = node->kind();
	 VarType leftType;
	 VarType rightType;
	 Label returnTrueLabel(_byteCode);
	 Label returnFalseLabel(_byteCode);
	 Label endLabel(_byteCode);
	 
	 node->left()->visit(this);
	 leftType = _operandType;
	 if (leftType == VT_INVALID || leftType == VT_STRING) {
		throw TranslationException("Invalid left operand type: only int or double types can be used for binary operations ");
	 }
	 
	 if (opKind == tOR && leftType == VT_INT) {
		_byteCode->addInsn(BC_ILOAD1);
		_byteCode->addBranch(BC_IFICMPE, returnTrueLabel);
	 }
	 
	 if (opKind == tAND && leftType == VT_INT) {
		_byteCode->addInsn(BC_ILOAD0);
		_byteCode->addBranch(BC_IFICMPE, returnFalseLabel);
	 }
	 
	 node->right()->visit(this);
	 rightType = _operandType;
	 if (rightType == VT_INVALID || rightType == VT_STRING) {
		throw TranslationException("Invalid right operand type: only int or double types can be used for binary operations ");
	 }
	 
	 if ((opKind == tOR || opKind == tAND) && rightType == VT_INT) {
		_byteCode->addInsn(BC_ILOAD1);
		_byteCode->addBranch(BC_IFICMPE, returnTrueLabel);
		_byteCode->addBranch(BC_JA, returnFalseLabel);
	 }
	 
	 if (leftType == VT_INT && rightType == VT_DOUBLE)
		_byteCode->addInsn(BC_D2I);
		
	 if (leftType ==  VT_DOUBLE && rightType == VT_INT)
		_byteCode->addInsn(BC_I2D);
	
	bool isInt = (leftType == VT_INT); 
	
	if (opKind == tEQ || opKind == tNEQ || opKind == tGT || 
			opKind == tGE || opKind == tLT || opKind == tLE) {
				
		isInt ? _byteCode->addInsn(BC_ICMP) : _byteCode->addInsn(BC_DCMP);
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
				_byteCode->addInsn(BC_ILOADM1);
				_byteCode->addBranch(BC_IFICMPE, returnTrueLabel);
				break;
			case tGE:
				_byteCode->addInsn(BC_ILOAD1);
				_byteCode->addBranch(BC_IFICMPNE, returnTrueLabel);
				break;
			case tLT:
				_byteCode->addInsn(BC_ILOAD1);
				_byteCode->addBranch(BC_IFICMPE, returnTrueLabel);
				break;
			case tLE:
				_byteCode->addInsn(BC_ILOADM1);
				_byteCode->addBranch(BC_IFICMPNE, returnTrueLabel);
				break;
			default:
				break;
		}
		_byteCode->addInsn(BC_ILOAD0);
		_byteCode->addBranch(BC_JA, endLabel);
	}

  if (opKind == tOR || opKind == tAND) {
    _byteCode->bind(returnFalseLabel);
    _byteCode->addInsn(BC_ILOAD0);
    _byteCode->addBranch(BC_JA, endLabel);

  }

	
	switch (opKind) {
		case tADD:
			isInt ? _byteCode->addInsn(BC_IADD): _byteCode->addInsn(BC_DADD);
			_byteCode->addBranch(BC_JA, endLabel);
			break;
		case tSUB:
			isInt ? _byteCode->addInsn(BC_ISUB): _byteCode->addInsn(BC_DSUB);
			_byteCode->addBranch(BC_JA, endLabel);
			break;
		case tMUL:
			isInt ? _byteCode->addInsn(BC_IMUL): _byteCode->addInsn(BC_DMUL);
			_byteCode->addBranch(BC_JA, endLabel);
			break;
		case tDIV:
			isInt ? _byteCode->addInsn(BC_IDIV): _byteCode->addInsn(BC_DDIV);
			_byteCode->addBranch(BC_JA, endLabel);
			break;
		default:
				break;
		
	}
	_byteCode->bind(returnTrueLabel);
	_byteCode->addInsn(BC_ILOAD1);
	_byteCode->bind(endLabel);
	 
 }
  
 void TranslateVisitor::visitDoubleLiteralNode(mathvm::DoubleLiteralNode* node) {
	_operandType = VT_DOUBLE;
	_byteCode->addInsn(BC_DLOAD);
	_byteCode->addDouble(node->literal()); 
	 
 }
 
 void TranslateVisitor::visitIntLiteralNode(mathvm::IntLiteralNode* node) {
	_operandType = VT_INT;
	_byteCode->addInsn(BC_ILOAD);
	_byteCode->addInt64(node->literal());
 }
 
 void TranslateVisitor::visitStringLiteralNode(mathvm::StringLiteralNode* node) {
	_operandType = VT_STRING;
	uint16_t strId = _code.makeStringConstant(node->literal());
	_byteCode->addInsn(BC_SLOAD);
	_byteCode->addUInt16(strId);
 }
 
 void TranslateVisitor::visitForNode(mathvm::ForNode* node) {
	VarType varType = node->var()->type();
	if (varType != VT_INT)
		throw TranslationException("Invalid variable operand type for expression: only int type can be used");
	std::string varName = node->var()->name();
	struct VarInfo varInfo = _scope->getVarIdByName(varName);
	bool theSameFunction = (varInfo._functionId == _scope->getFunctionInfo._id);
	Instruction insn = BC_INVALID;
		
	BinaryOpNode* inBinOp = dynamic_cast<BinaryOpNode*>(node->inExpr());
	if (!inBinOp || inBinOp->kind() != tRANGE) 
		throw TranslationException("Invalid binary operation for expression: only range operation can be used in for expression");
	
	inBinOp->left()->visit(this);
	if (_operandType != VT_INT)
		throw TranslationException("Invalid left operand type: only int type can be used for range operation");
		 
	(theSameFunction) ? insn  = BC_STOREIVAR : insn = BC_STORECTXIVAR;
	_byteCode.addInsn(insn);
	if (!theSameFunction) _byteCode->addUInt16(varInfo._functionId);
	_byteCode->addUInt16(varInfo._id);
	
	Label startLoopLabel(_byteCode), endLabel(_byteCode);
	_byteCode->bind(startLoopLabel);
	inBinOp->right()->visit(this);
	if (_operandType != VT_INT) 
		throw TranslationException("Invalid right operand type: only int type can be used for range operation");
	(theSameFunction) ? insn  = BC_LOADIVAR : insn = BC_LOADCTXIVAR;
	_byteCode.addInsn(insn);
	if (!theSameFunction) _byteCode->addUInt16(varInfo._functionId);
	_byteCode->addUInt16(varInfo._id);
	
	_byteCode->addBranch(BC_IFICMPL, endLabel);
	node->body()->visit(this);
	
	(theSameFunction) ? insn  = BC_LOADIVAR : insn = BC_LOADCTXIVAR;
	_byteCode.addInsn(insn);
	if (!theSameFunction) _byteCode->addUInt16(varInfo._functionId);
	_byteCode->addUInt16(varInfo._id);
	_byteCode->addInsn(BC_ILOAD1);
	_byteCode->addInsn(BC_IADD);
	(theSameFunction) ? insn  = BC_STOREIVAR : insn = BC_STORECTXIVAR;
	_byteCode.addInsn(insn);
	if (!theSameFunction) _byteCode->addUInt16(varInfo._functionId);
	_byteCode->addUInt16(varInfo._id);
	_byteCode->addBranch(BC_JA, startLoopLabel);
	_byteCode->bind(endLabel);
 }
 
 void TranslateVisitor::visitPrintNode(mathvm::PrintNode* node) {
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
  
 void TranslateVisitor::visitFunctionNode(mathvm::FunctionNode* node) {
	//executes only for top function 
	if (!_code.functionByName(node->name())) { 
		mathvm::BytecodeFunction *bcFunc = new mathvm::BytecodeFunction(extractAstFunction(node));
		bcFunc->setLocalsNumber(calculateFuncMaxLocals(node));
		_code.addFunction(bcFunc);
	}
	//save previous environment
	mathvm::ByteCode *prevByteCode = _byteCode;
	_byteCode = new mathvm::ByteCode();
	ScopeWrapper *parentScope = _scope;
	_scope = new ScopeWrapper(parentScope, true, node->name(), false); 
	_operandType = mathvm::VT_INVALID;
	//process function body 
	node->body()->visit(this);
	if (prevByteCode == NULL) _byteCode->addInsn(BC_STOP);
	//add generated bytecode for the function into the corresponding TranslatedFunction from _code
	mathvm::BytecodeFunction *bcFunc = _code.functionByName(node->name());
	bcFunc->setLocalsNumber(calculateFuncMaxLocals(node));
	*(bcFunc->bytecode()) = *_byteCode;
	//free needless structures	
	delete _byteCode;
	_byteCode = prevByteCode;
	delete _scope;
	_scope = parentScope; 
 }

 void TranslateVisitor::visit( mathvm::BlockNode * rootNode )
 {
   rootNode->visit(this);
 }

 void TranslateVisitor::visitCallNode(CallNode* node) {
	for (int i =0; i < node->parametersNumber(); ++i) {
		node->parameterAt(i)->visit(this);
	}
	_byteCode->addInsn(BC_CALL);
	mathvm::TranstlatedFunction *callFunction = _code.functionByName(node->name());
	_byteCode->addUInt16(callFunction->id());
 }

 void TranslateVisitor::visitReturnNode(ReturnNode* node) {
	if (node->returnExpr()) node->returnExpr()->visit(this);
	_byteCode->addInsn(BC_RETURN);
 }

 mathvm::Bytecode* TranslateVisitor::getBytecode()
 {
   return &_byteCode;
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
   _byteCode.dump();
 }
