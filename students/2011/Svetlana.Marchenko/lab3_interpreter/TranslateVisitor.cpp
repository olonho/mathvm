 #include "TranslateVisitor.h"
 
 using namespace mathvm;
 
 void TranslateVisitor::visitBlockNode(mathvm::BlockNode* node) {
	 Scope* blockScope = node->scope();
	 Scope::VarIterator varIt(blockScope);
   while (varIt.hasNext()) {
     AstVar* astVar = varIt.next();
     _varTable.addVar(astVar->name());
   }
	 node->visitChildren(this);
 }
 
 void TranslateVisitor::visitIfNode(mathvm::IfNode* node) {
	 Label thenLabel(&_byteCode);
	 Label endLabel(&_byteCode);
	 
	 node->ifExpr()->visit(this);
	 _byteCode.addInsn(BC_ILOAD1);
	 _byteCode.addBranch(BC_IFICMPE, thenLabel);
	 
	 if (node->elseBlock() != NULL) {
		 node->elseBlock()->visit(this);
		 _byteCode.addBranch(BC_JA, endLabel);
	 }
	 _byteCode.bind(thenLabel);
	 node->thenBlock()->visit(this);
	 _byteCode.addBranch(BC_JA, endLabel);
	 _byteCode.bind(endLabel); 
	 
 }
 
 void TranslateVisitor::visitWhileNode(mathvm::WhileNode* node) {
	 Label endLabel(&_byteCode);
	 Label loopLabel(&_byteCode);
	 
	 _byteCode.bind(loopLabel);
	 node->whileExpr()->visit(this);
	 _byteCode.addInsn(BC_ILOAD0);
	 _byteCode.addBranch(BC_IFICMPE, endLabel);
	 node->loopBlock()->visit(this);
	 _byteCode.addBranch(BC_JA, loopLabel);
	 _byteCode.bind(endLabel);
 }
 
  void TranslateVisitor::visitLoadNode(mathvm::LoadNode* node) {
	  VarType type = node->var()->type();
	  int varId = _varTable.getIdByName(node->var()->name());
    _operandType = type;
	  switch (type) {
		  case VT_INVALID:
		    throw TranslationException("Type of variable is invalid");
			break;
		  case VT_DOUBLE: 
			_byteCode.addInsn(BC_LOADDVAR);
			break;
		  case VT_INT:
			_byteCode.addInsn(BC_LOADIVAR);
			break;
		  case VT_STRING:
			_byteCode.addInsn(BC_LOADSVAR);
			break;
		  default:
			break;
	  }
	  _byteCode.addByte((uint8_t)varId);
  }
  
  void TranslateVisitor::visitStoreNode(mathvm::StoreNode* node) {
	  VarType varType = node->var()->type();
	  TokenKind tokenKind = node->op();
	  AstNode* valueNode = node->value();
	  int varId = _varTable.getIdByName(node->var()->name());
	  
	  switch (varType) {
		  case VT_STRING:
        valueNode->visit(this);
			  if ((_operandType == VT_STRING) 
					&& (tokenKind == tASSIGN)) {
				
				_byteCode.addInsn(BC_STORESVAR);
				_byteCode.addByte((uint8_t)varId);
			} else {
				throw TranslationException("Couldn`t process any other operations with strings except assign");
			}
		    break;
		  case VT_INT:
			  valueNode->visit(this);
			  if (_operandType==VT_INT || _operandType==VT_DOUBLE) {
				
				if (valueNode->isDoubleLiteralNode())
					_byteCode.addInsn(BC_D2I);
				switch (tokenKind) {
					case tASSIGN:
						break;
					case tINCRSET:
						_byteCode.addInsn(BC_LOADIVAR);
						_byteCode.addByte((uint8_t)varId);
						_byteCode.addInsn(BC_IADD);
						break;
					case tDECRSET:
						_byteCode.addInsn(BC_LOADIVAR);
						_byteCode.addByte((uint8_t)varId);
						_byteCode.addInsn(BC_SWAP);
						_byteCode.addInsn(BC_ISUB);
						break;
					default:
						break;
				}
				_byteCode.addInsn(BC_STOREIVAR);
				_byteCode.addByte((uint8_t)varId);
			} else {
				throw TranslationException("Couldn`t store into int variable: Invalid operands type to store");
			}
			break;
		  case VT_DOUBLE:
        valueNode->visit(this);
			  if (_operandType == VT_DOUBLE || _operandType == VT_INT) {
				
				if (valueNode->isIntLiteralNode())
					_byteCode.addInsn(BC_I2D);
				switch (tokenKind) {
					case tASSIGN:
						break;
					case tINCRSET:
						_byteCode.addInsn(BC_LOADDVAR);
						_byteCode.addByte((uint8_t)varId);
						_byteCode.addInsn(BC_DADD);
						break;
					case tDECRSET:
						_byteCode.addInsn(BC_LOADDVAR);
						_byteCode.addByte((uint8_t)varId);
						_byteCode.addInsn(BC_DSUB);
						break;
					default:
						break;
				}
				_byteCode.addInsn(BC_STOREDVAR);
				_byteCode.addByte((uint8_t)varId);
			}
			break;
		  case VT_INVALID:
			throw TranslationException("Couldn`t store into double variable: Invalid operands type to store");
			break;
		  default:;
		  }
 
  }
 
  void TranslateVisitor::visitUnaryOpNode(mathvm::UnaryOpNode* node) {
	  TokenKind kind = node->kind();
	  bool isInt = node->operand()->isIntLiteralNode();
	  if (isInt || node->operand()->isDoubleLiteralNode()) {
		  switch (kind) {
			  case tSUB:
				node->visitChildren(this);
				isInt ? _byteCode.addInsn(BC_INEG): _byteCode.addInsn(BC_DNEG);
				break;
			  case tNOT:
				if (isInt) {
					Label thenLabel(&_byteCode);
					Label endLabel(&_byteCode);
					node->visitChildren(this);
					_byteCode.addInsn(BC_ILOAD0);
					_byteCode.addBranch(BC_IFICMPE, thenLabel);
					_byteCode.addInsn(BC_ILOAD1);
					_byteCode.addBranch(BC_JA, endLabel);
					_byteCode.bind(thenLabel);
					_byteCode.addInsn(BC_ILOAD0);
					_byteCode.bind(endLabel);					
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
	 Label returnTrueLabel(&_byteCode);
	 Label returnFalseLabel(&_byteCode);
	 Label endLabel(&_byteCode);
	 
	 node->left()->visit(this);
	 leftType = _operandType;
	 if (leftType == VT_INVALID || leftType == VT_STRING) {
		throw TranslationException("Invalid left operand type: only int or double types can be used for binary operations ");
	 }
	 
	 if (opKind == tOR && leftType == VT_INT) {
		_byteCode.addInsn(BC_ILOAD1);
		_byteCode.addBranch(BC_IFICMPE, returnTrueLabel);
	 }
	 
	 if (opKind == tAND && leftType == VT_INT) {
		_byteCode.addInsn(BC_ILOAD0);
		_byteCode.addBranch(BC_IFICMPE, returnFalseLabel);
	 }
	 
	 node->right()->visit(this);
	 rightType = _operandType;
	 if (rightType == VT_INVALID || rightType == VT_STRING) {
		throw TranslationException("Invalid right operand type: only int or double types can be used for binary operations ");
	 }
	 
	 if ((opKind == tOR || opKind == tAND) && rightType == VT_INT) {
		_byteCode.addInsn(BC_ILOAD1);
		_byteCode.addBranch(BC_IFICMPE, returnTrueLabel);
		_byteCode.addBranch(BC_JA, returnFalseLabel);
	 }
	 
	 if (leftType == VT_INT && rightType == VT_DOUBLE)
		_byteCode.addInsn(BC_D2I);
		
	 if (leftType ==  VT_DOUBLE && rightType == VT_INT)
		_byteCode.addInsn(BC_I2D);
	
	bool isInt = (leftType == VT_INT); 
	
	if (opKind == tEQ || opKind == tNEQ || opKind == tGT || 
			opKind == tGE || opKind == tLT || opKind == tLE) {
				
		isInt ? _byteCode.addInsn(BC_ICMP) : _byteCode.addInsn(BC_DCMP);
		switch (opKind) {
			case tEQ:
				_byteCode.addInsn(BC_ILOAD0);
				_byteCode.addBranch(BC_IFICMPE, returnTrueLabel);
				break;
			case tNEQ:
				_byteCode.addInsn(BC_ILOAD0);
				_byteCode.addBranch(BC_IFICMPNE, returnTrueLabel);
				break;
			case tGT:
				_byteCode.addInsn(BC_ILOADM1);
				_byteCode.addBranch(BC_IFICMPE, returnTrueLabel);
				break;
			case tGE:
				_byteCode.addInsn(BC_ILOAD1);
				_byteCode.addBranch(BC_IFICMPNE, returnTrueLabel);
				break;
			case tLT:
				_byteCode.addInsn(BC_ILOAD1);
				_byteCode.addBranch(BC_IFICMPE, returnTrueLabel);
				break;
			case tLE:
				_byteCode.addInsn(BC_ILOADM1);
				_byteCode.addBranch(BC_IFICMPNE, returnTrueLabel);
				break;
			default:
				break;
		}
		_byteCode.addInsn(BC_ILOAD0);
		_byteCode.addBranch(BC_JA, endLabel);
	}

  if (opKind == tOR || opKind == tAND) {
    _byteCode.bind(returnFalseLabel);
    _byteCode.addInsn(BC_ILOAD0);
    _byteCode.addBranch(BC_JA, endLabel);

  }

	
	switch (opKind) {
		case tADD:
			isInt ? _byteCode.addInsn(BC_IADD): _byteCode.addInsn(BC_DADD);
			_byteCode.addBranch(BC_JA, endLabel);
			break;
		case tSUB:
			isInt ? _byteCode.addInsn(BC_ISUB): _byteCode.addInsn(BC_DSUB);
			_byteCode.addBranch(BC_JA, endLabel);
			break;
		case tMUL:
			isInt ? _byteCode.addInsn(BC_IMUL): _byteCode.addInsn(BC_DMUL);
			_byteCode.addBranch(BC_JA, endLabel);
			break;
		case tDIV:
			isInt ? _byteCode.addInsn(BC_IDIV): _byteCode.addInsn(BC_DDIV);
			_byteCode.addBranch(BC_JA, endLabel);
			break;
		default:
				break;
		
	}
	_byteCode.bind(returnTrueLabel);
	_byteCode.addInsn(BC_ILOAD1);
	_byteCode.bind(endLabel);
	 
 }
  
 void TranslateVisitor::visitDoubleLiteralNode(mathvm::DoubleLiteralNode* node) {
	_operandType = VT_DOUBLE;
	_byteCode.addInsn(BC_DLOAD);
	_byteCode.addDouble(node->literal()); 
	 
 }
 
 void TranslateVisitor::visitIntLiteralNode(mathvm::IntLiteralNode* node) {
	_operandType = VT_INT;
	_byteCode.addInsn(BC_ILOAD);
	_byteCode.addInt64(node->literal());
 }
 
 void TranslateVisitor::visitStringLiteralNode(mathvm::StringLiteralNode* node) {
	_operandType = VT_STRING;
	uint16_t strId = _code.makeStringConstant(node->literal());
	_byteCode.addInsn(BC_SLOAD);
	_byteCode.addInt16(strId);
 }
 
 void TranslateVisitor::visitForNode(mathvm::ForNode* node) {
	VarType varType = node->var()->type();
	if (varType != VT_INT)
		throw TranslationException("Invalid variable operand type for expression: only int type can be used");
	std::string varName = node->var()->name();
	uint8_t varId = (uint8_t)_varTable.getIdByName(varName);
	
	
	BinaryOpNode* inBinOp = dynamic_cast<BinaryOpNode*>(node->inExpr());
	if (!inBinOp || inBinOp->kind() != tRANGE) 
		throw TranslationException("Invalid binary operation for expression: only range operation can be used in for expression");
	
	inBinOp->left()->visit(this);
	if (_operandType != VT_INT)
		throw TranslationException("Invalid left operand type: only int type can be used for range operation");
		 
	_byteCode.addInsn(BC_STOREIVAR);
	_byteCode.addByte(varId);
	
	Label startLoopLabel(&_byteCode), endLabel(&_byteCode);
	_byteCode.bind(startLoopLabel);
	inBinOp->right()->visit(this);
	if (_operandType != VT_INT) 
		throw TranslationException("Invalid right operand type: only int type can be used for range operation");
	_byteCode.addInsn(BC_LOADIVAR);
	_byteCode.addByte(varId);
	_byteCode.addBranch(BC_IFICMPL, endLabel);
	node->body()->visit(this);
  _byteCode.addInsn(BC_LOADIVAR);
  _byteCode.addByte(varId);
  _byteCode.addInsn(BC_ILOAD1);
  _byteCode.addInsn(BC_IADD);
  _byteCode.addInsn(BC_STOREIVAR);
  _byteCode.addByte(varId);
	_byteCode.addBranch(BC_JA, startLoopLabel);
	_byteCode.bind(endLabel);
 }
 
 void TranslateVisitor::visitPrintNode(mathvm::PrintNode* node) {
	for(size_t i = 0; i < node->operands(); ++i) {
		AstNode* astNode = node->operandAt(i);
    astNode->visit(this);
    switch(_operandType) {
      
      case VT_STRING : _byteCode.addInsn(BC_SPRINT); break;
      case VT_INT : _byteCode.addInsn(BC_IPRINT); break;
      case VT_DOUBLE : _byteCode.addInsn(BC_DPRINT); break;
      default:;
    }
	}
 }
  
 void TranslateVisitor::visitFunctionNode(mathvm::FunctionNode* node) {}

 void TranslateVisitor::visit( mathvm::BlockNode * rootNode )
 {
   rootNode->visit(this);
 }

 void TranslateVisitor::visitCallNode(CallNode* node) {}

 void TranslateVisitor::visitReturnNode(ReturnNode* node) {}

 mathvm::Bytecode* TranslateVisitor::GetBytecode()
 {
   return &_byteCode;
 }

 std::vector<std::string> TranslateVisitor::GetStringsVector()
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
