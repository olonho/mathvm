#include "BytecodeTranslatorImpl.h"
#include "ast.h"

void TranslatorImpl::initVisit(AstNode* node) {
    AstNodeHierarchy* newNode = new AstNodeHierarchy();
    newNode->parent = currentHierarchy;
    newNode->node = node;
    currentHierarchy->children.push_back(newNode);
    currentHierarchy = newNode;
}

void TranslatorImpl::endVisit(AstNode* node) {
    currentHierarchy = currentHierarchy->parent;
}

void TranslatorImpl::visitBinaryOpNode(BinaryOpNode* node) {  
    
    initVisit(node);
  
    Label success(byteCode);
    Label end(byteCode);
  
    node->left->visit(this);
    VarType leftType = currentType;
    node->right->visit(this);
    VarType rightType = currentType;
  
    switch (node->kind()) {
      case tOR:	
	assertIntTypes(leftType, rightType);
	pushInstruction(BC_IAOR);
	currentType = VT_INT;
	break;
      case tAND:
	assertIntTypes(leftType, rightType);
	pushInstruction(BC_IAAND);
	currentType = VT_INT;
	break;
      case tAOR:
	assertIntTypes(leftType, rightType);
	pushInstruction(BC_IAOR);
	currentType = VT_INT;
	break;
      case tAAND:
	assertIntTypes(leftType, rightType);
	pushInstruction(BC_IAAND);
	currentType = VT_INT;
	break;
      case tAXOR:
	assertIntTypes(leftType, rightType);
	pushInstruction(BC_IAXOR);
	currentType = VT_INT;
	break;
      case tNEQ:
	pushCmp(leftType, rightType);
	currentType = VT_INT;
	break;
      case tEQ:
	pushCmp(leftType, rightType);
	pushInstruction(BC_ILOAD1);
	pushInstruction(BC_IAAND);
	pushInstruction(BC_ILOAD1);
	pushInstruction(BC_IAXOR);
	currentType = VT_INT;
	break;
      case tGT:
	pushCmp(leftType, rightType);
	pushInstruction(BC_ILOAD1);
	pushInstruction(BC_ISUB);
	currentType = VT_INT;
	break;
      case tGE:
	pushCmp(leftType, rightType);
	pushInstruction(BC_ILOAD0);
	byteCode->addBranch(BC_IFICMPLE, success);
	pushInstruction(BC_ILOAD0);
	byteCode->addBranch(BC_JA, end);
	byteCode->bind(success);
	pushInstruction(BC_ILOAD1);
	byteCode->bind(end);
	currentType = VT_INT;
	break;
      case tLT:
	pushCmp(leftType, rightType);
	pushInstruction(BC_ILOAD1);
	pushInstruction(BC_IADD);	
	currentType = VT_INT;
	break;
      case tLE:
	pushCmp(leftType, rightType);
	pushInstruction(BC_ILOAD0);
        byteCode->addBranch(BC_IFICMPGE, success);
	pushInstruction(BC_ILOAD0);
	byteCode->addBranch(BC_JA, end);
	byteCode->bind(success);
	pushInstruction(BC_ILOAD1);
	byteCode->bind(end);
	currentType = VT_INT;
	break;
      case tADD:
	currentType = pushAdd(leftType, rightType);
	break;
      case tSUB:
	currentType = pushSub(leftType, rightType);
	break;
      case tMUL:
	currentType = pushMul(leftType, rightType);
	break;
      case tDIV:
	currentType = pushDiv(leftType, rightType);
	break;
      case tMOD:
	assertIntTypes(leftType, rightType);
	pushInstruction(BC_IMOD);
	currentType = VT_INT;
	break;
      default:
	throw translationException;
    }
    
    endVisit(node);
}

void TranslatorImpl::visitUnaryOpNode(UnaryOpNode* node) {
  initVisit(node);
  
  node->operand()->visit(this);
  
  switch (node->kind()) {
    case tNOT:
      assertIntType(currentType);
      pushInstruction(BC_ILOAD1);
      pushInstruction(BC_IAAND);
      pushInstruction(BC_ILOAD1);
      pushInstruction(BC_IAXOR);
      currentType = VT_INT;
      break;
    case tSUB:
      assertNumericType(currentType);
      currentType = pushUnaryMinus(currentType);
      break;
    default:
      throw 0;
    }
    
    endVisit(node);
}

void TranslatorImpl::visitStringLiteralNode(StringLiteralNode* node) {        
  initVisit(node);
  pushInstruction(BC_SLOAD);
  byteCode->addUInt16(code->makeStringConstant(node->literal()));
  currentType = VT_STRING;
  endVisit(node);
}

void TranslatorImpl::visitDoubleLiteralNode(DoubleLiteralNode* node) {
  initVisit(node);      
  byteCode->addDouble(node->literal());
  currentType = VT_DOUBLE;
  endVisit(node);
}

void TranslatorImpl::visitIntLiteralNode(IntLiteralNode* node) {
  initVisit(node);
  byteCode->addInt64(node->literal());
  currentType = VT_INT;
  endVisit(node);
}
    
void TranslatorImpl::visitLoadNode(LoadNode* node) {
  initVisit(node);
  AstVar* var = node->var();
  var->setInfo(varInfo(varId, currentScope));
  varId++;
  currentType = VT_VOID;
  endVisit(node);
}

void TranslatorImpl::visitStoreNode(StoreNode* node) {
  initVisit(node);
  AstVar* var = node->var();
  VarType varType = var->type();
  AstNode* value = node->value();
  value->visit(this);
  VarType valueType = currentType;
  assertAssignable(var, valueType);
  switch (node->op()) {
      case tINCRSET: 
	loadFromVar(var);
	currentType = pushAdd(varType, valueType);
	storeIntoVar(var);
	loadFromVar(var);
	break;
      case tDECRSET:	
	loadFromVar(var);
	pushInstruction(BC_SWAP);
	currentType = pushSub(varType, valueType);
	storeIntoVar(var);
	loadFromVar(var);
	break;
      case tASSIGN:
	currentType = varType;
	assertAssignable(varType, valueType);
	storeIntoVar(var);
      default:
	throw translationException;
  }
  endVisit(node);
}

void TranslatorImpl::visitForNode(ForNode* node) {
    initVisit(node);  
    AstVar* var = node->var();
    AstNode* condition = node->inExpr();
    BlockNode* body = node->body();
    VarType varType = var->type();
    
    Label loopStart(byteCode);
    Label loopEnd(byteCode);
    
    if (!condition->isBinaryOpNode() || condition->asBinaryOpNode()->kind() != tRANGE) {
      throw translationException;
    }
    
    BinaryOpNode* range = condition->asBinaryOpNode();
    
    uint16_t forScope = currentScope;
    uint16_t forEndVarId = varId++;
    
    range->left()->visit();
    VarType leftType = currentType;
    
    assertSameTypes(leftType, varType);
    
    storeIntoVar(var);
    
    range->right()->visit();
    VarType rightType = currentType;
    assertSameTypes(leftType, rightType);
    
    storeIntoVarById(forEndVarId, forScope, varType);
    
    byteCode->bind(loopStart);
    
    loadFromVar(var);
    loadFromVarById(forEndVarId, forScope, varType);
    
    byteCode->addBranch(BC_IFICMPE, loopEnd);    
    body->visit(this);    
    byteCode->addBranch(BC_JA, loopStart);    
    byteCode->bind(loopEnd);            
    
    varId--;
    
    currentType = VT_VOID;
    
    endVisit(node);
}

void TranslatorImpl::visitWhileNode(WhileNode* node) {
  
  initVisit(node);
  Label loopStart(byteCode);
  Label loopEnd(byteCode);
  byteCode->bind(loopStart);
  node->whileExpr()->visit(this);
  assertIntType(currentType);
  pushInstruction(BC_ILOAD0);
  byteCode->addBranch(BC_IFICMPE, loopEnd);
  node->loopBlock()->visit(this);
  byteCode->addBranch(BC_JA, loopStart);
  byteCode->bind(loopEnd);  
  currentType = VT_VOID;  
  endVisit(node);
}

void TranslatorImpl::visitIfNode(IfNode* node) {
  initVisit(node);
  Label elseLabel(byteCode);
  Label endLabel(byteCode);

  if (!node->elseBlock()) {
    node->ifExpr()->visit(this);
    assertIntType(currentType);
    pushInstruction(BC_ILOAD0);
    byteCode->addBranch(BC_IFICMPE, endLabel);
    node->thenBlock()->visit(this);
    byteCode->bind(endLabel);
  } else {
    node->ifExpr()->visit(this);
    assertIntType(currentType);
    pushInstruction(BC_ILOAD0);
    byteCode->addBranch(BC_IFICMPE, elseLabel);
    node->thenBlock()->visit(this);
    byteCode->addBranch(BC_JA, endLabel);
    byteCode->bind(elseLabel);
    node->elseBlock()->visit(this);
    byteCode->bind(endLabel);
  }
  endVisit(node);
}

void TranslatorImpl::visitBlockNode(BlockNode* node) {
  initVisit(node);
  uint16_t scopeOuter = currentScope;
  currentScope = ++scopeId;
  uint16_t varIdOuter = varId;
  varId = 0;
  
  std::map<Scope*, AstFunction*>::iterator implementedFun = implToFunction.find(node->scope());
  if (implementedFun != implToFunction.end()) {
    Scope* scope = node->scope();
    AstFunction* fun = scope->lookupVariable();
    for (uint32_t i = 0; i < fun->parametersNumber(); ++i) {
      AstVar * parameter = scope->lookupVariable(fun->parameterName(i));
      parameter->setInfo(varInfo(varId++, currentScope));
      storeIntoVar(parameter);
    }
  
    fun->node()->visit(this);
    
    //maybe store/load results of function calculation?
    
    pushInstruction(BC_RETURN);
  }
   
  Scope::VarIterator varIterator(node->scope());
  
  while (varIterator.hasNext()) {
    AstVar* nextVar = varIterator.next();
    nextVar->setInfo(varInfo(varId++, currentScope));
  }
  
  Scope::FunctionIterator funIterator(node->scope());
  
  while (funIterator.hasNext()) {
    
    AstFunction* nextFun = funIterator.next();    
    
    Scope *scope = nextFun->node()->body()->scope();
    
    implToFunction[scope] = nextFun;
    Function funRecord;
    funRecord.id = funId++;
    funRecord.node = nextFun;
    (namesToFunctionIds[node->scope()])[nextFun->name()] = funRecord;
  }
  
  for (uint32_t i = 0; i < node->nodes(); ++i) {
    node->nodeAt(i)->visit(this);
  }
  //TODO: replace with stack
  currentScope = scopeOuter;
  varId = varIdOuter;
  endVisit(node);
}

void TranslatorImpl::visitReturnNode(ReturnNode* node) {
  initVisit(node);
  if (node->returnExpr()) {
    node->returnExpr()->visit(this);
  }
  pushInstruction(BC_RETURN);
  endVisit(node);
}

void TranslatorImpl::visitFunctionNode(FunctionNode *node) {
  node->visitChildren(this);
}

void TranslatorImpl::visitCallNode(CallNode* node) {
  initVisit(node);
  Function callee = findFunction(node->name(), node);
  if (callee == NULL) {
    throw translationException;
  }
  for (uint32_t i = 0; i < node->parametersNumber(); ++i) {
    node->parameterAt(i)->visit(this);
  }
  
  pushInstruction(BC_CALL);
  byteCode->addUInt16(callee.id);
  
/*
  uint16_t returnVarId = varId++;
  storeIntoVarById(returnVarId, currentScope, callee.node->returnType());
  
  for (uint32_t i = 0; i < node->parametersNumber(); ++i) {
    pushInstruction(BC_POP);
  }
  
  loadFromVarById(returnVarId, currentScope, callee.node->returnType());
  
  varId--; */
  
  currentType = callee->node->returnType();
  endVisit(node);
}

void TranslatorImpl::visitNativeCallNode(NativeCallNode* node) {
  initVisit(node);
  //TODO: implement if it is required
  endVisit(node);
}

void TranslatorImpl::visitPrintNode(PrintNode* node) {
  initVisit(node);
  for (uint32_t i = 0; i < node->operands(); ++i) {
    node->operandAt(i)->visit(this);
    pushInstruction(BC_IPRINT, currentType);
  }
  currentType = VT_INVALID;
  endVisit(node);
}



void TranslatorImpl::pushCmp(VarType leftType, VarType rightType) {
  assertSameTypes(leftType, rightType);	
    if (leftType == VT_INT) {
      pushInstruction(BC_ICMP);	  
    } else if (leftType == VT_DOUBLE) {
      pushInstruction(BC_DCMP);
    } else throw translationException;
}

VarType TranslatorImpl::pushAdd(VarType leftType, VarType rightType) {    
  if (leftType == VT_INT && rightType == VT_INT) {
      pushInstruction(BC_IADD);
      return VT_INT;
  }
  if (leftType == VT_DOUBLE) {
      pushInstruction(BC_I2D);
  }
  if (rightType == VT_DOUBLE) {
      pushInstruction(BC_SWAP);
      pushInstruction(BC_I2D);
  }
  pushInstruction(BC_DADD);
  return VT_DOUBLE;
}

VarType TranslatorImpl::pushSub(VarType leftType, VarType rightType) {
  pushInstruction(BC_SWAP);  
  if (leftType == VT_INT && rightType == VT_INT) {
	pushInstruction(BC_ISUB);
	return VT_INT;
    }
  if (right == VT_DOUBLE) {
    pushInstruction(BC_I2D);
  }
  if (left == VT_DOUBLE) {
      pushInstruction(BC_SWAP);
      pushInstruction(BC_I2D);
      pushInstruction(BC_SWAP);
  }
  pushInstruction(BC_DSUB);
  return VT_DOUBLE;
}

VarType TranslatorImpl::pushMul(VarType leftType, VarType rightType) {
  if (leftType == VT_INT && rightType == VT_INT) {
      pushInstruction(BC_IMUL);
      return VT_INT;
  }
  if (right == VT_DOUBLE) {
   pushInstruction(BC_I2D); 
  }
  if (left == VT_DOUBLE) {
      pushInstruction(BC_SWAP);
      pushInstruction(BC_I2D);
  }
  pushInstruction(BC_DMUL);
  return VT_DOUBLE;
}

VarType TranslatorImpl::pushDiv(VarType leftType, VarType rightType) {
  if (leftType == VT_INT && rightType == VT_INT) {
    pushInstruction(BC_IDIV);
    return VT_INT;
  }
  if (right == VT_DOUBLE) {
    pushInstruction(BC_I2D);
  }
  if (left == VT_DOUBLE) {
    pushInstruction(BC_SWAP);
    pushInstruction(BC_I2D);
    pushInstruction(BC_SWAP);
  }
  pushInstruction(BC_DDIV);
  return VT_DOUBLE;
}

VarType TranslatorImpl::pushUnaryMinus(VarType type) {
  if (type == VT_INT) {
    pushInstruction(BC_INEG);
    return VT_INT;
  } else if (type == VT_DOUBLE) {
    pushInstruction(BC_DNEG);
    return VT_DOUBLE;
  } else throw translationException;
}

void TranslatorImpl::assertIntType(VarType type) {
    if (type != VT_INT) {
      throw translationException;
    }
}

void TranslatorImpl::assertNumericType(VarType type) {
    if (type != VT_INT && type != VT_DOUBLE) {
      throw translationException;
    }
}

void TranslatorImpl::assertSameTypes(VarType left, VarType right) {
  if (left != right) {
    throw translationException;        VarType pushMul(VarType leftType, VarType rightType);
    VarType pushDiv(VarType leftType, VarType rightType);
  }
}

void TranslatorImpl::assertIntTypes(VarType left, VarType right) {
  if (left != VT_INT || right != VT_INT) {
    throw translationException;
  }
}

void TranslatorImpl::assertAssignable(VarType to, VarType from) {
  if ((to == from) || (to == VT_DOUBLE && from == VT_INT)) return;
  throw translationException;
}

void TranslatorImpl::storeIntoVarById(uint16_t varId, uint16_t scopeId, VarType varType) {
  if (scopeId != currentScope) {
    if (type == VT_INT) {
      pushInstruction(BC_STORECTXIVAR);
    } else if (type == VT_DOUBLE) {
      pushInstruction(BC_STORECTXDVAR);
    } else if (type == VT_STRING) {
      pushInstruction(BC_STORECTXSVAR);
    } else throw translationException;
    byteCode->addUInt16(scopeId);
    byteCode->addUInt16(varId);
  } else {
    if (type == VT_INT) {
      pushInstruction(BC_STOREIVAR);
    } else if (type == VT_DOUBLE) {
      pushInstruction(BC_STOREDVAR);
    } else if (type == VT_STRING) {
      pushInstruction(BC_STORESVAR);
    } else throw translationException;
    byteCode->addUInt16(varId);
  }
}

void TranslatorImpl::storeIntoVar(AstVar* var) {
  uint32_t id = &((uint32_t) var->info());
  uint16_t scopeId = id >> 16;
  uint16_t varId = id & ((1 << 16) - 1);
  VarType type = var->type();
  storeIntoVarById(varId, scopeId, type);
}

void TranslatorImpl::loadFromVar(AstVar* var) {
  uint32_t id = &((uint32_t) var->info());
  uint16_t scopeId = id >> 16;
  uint16_t varId = id & ((1 << 16) - 1);
  VarType type = var->type();
  loadFromVarById(varId, scopeId, varType);
}

void TranslatorImpl::loadFromVarById(uint16_t varId, uint16_t scopeId, VarType varType) {
   if (scopeId != currentScope) {
    if (type == VT_INT) {
      pushInstruction(BC_LOADCTXIVAR);
    } else if (type == VT_DOUBLE) {
      pushInstruction(BC_LOADCTXDVAR);
    } else if (type == VT_STRING) {
      pushInstruction(BC_LOADCTXSVAR);
    } else throw translationException;
    byteCode->addUInt16(scopeId);
    byteCode->addUInt16(varId);
  } else {
    if (type == VT_INT) {
      pushInstruction(BC_LOADIVAR);
    } else if (type == VT_DOUBLE) {
      pushInstruction(BC_LOADDVAR);
    } else if (type == VT_STRING) {
      pushInstruction(BC_LOADSVAR);
    } else throw translationException;
    byteCode->addUInt16(varId);
  } 
}

uint32_t TranslatorImpl::varInfo(uint16_t varId, uint16_t scopeId) {
    return (scopeId << 16) + varId;
}

Function TranslatorImpl::findFunction(std::string& name, AstNodeHierarchy* currentNode) {
    AstNodeHierarchy* searchNode = currentNode;
    while (searchNode != NULL) {
      if (searchNode->node->isBlockNode()) {
	Scope* scope = searchNode->node->asBlockNode()->scope();
	map<String&, Function>::iterator it = namesToFunctionIds[scope].find(name);
	if (it != namesToFunctionIds[scope].end()) {
	  return (*(namesToFunctionIds[scope])[name]);
	}
      }
      searchNode = searchNode->parent;
    }
    return NULL;
}

TranslatorImpl::~TranslatorImpl() {
    rootHierarchy->node->visitChildren(DeleteVisitor());
}






















