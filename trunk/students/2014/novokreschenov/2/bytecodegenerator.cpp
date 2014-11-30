#include "bytecodegenerator.h"


namespace mathvm {

BytecodeGenerator::BytecodeGenerator()
    : _lastType(VT_INVALID) {
}

BytecodeGenerator::~BytecodeGenerator() {
}

Bytecode* BytecodeGenerator::bytecode() {
    return _bytecodeStack.back();
}

VarType BytecodeGenerator::lastInferredType() {
    return _lastType;
}

void BytecodeGenerator::refreshCurrentPosition(AstNode *node) {
    _currentPosition = node->position();
}

uint32_t BytecodeGenerator::currentPosition() {
    return _currentPosition;
}

void BytecodeGenerator::checkNumber(VarType type) {
    if (type != VT_DOUBLE && type != VT_INT) {
        throw TranslationException("Incorrect type: expected = \"int\" | \"double\", actual = \"" + string(typeToName(type)) + "\"",
                                   currentPosition());
    }
}

VarType BytecodeGenerator::getCommonType(VarType type1, VarType type2) {
    return TypeInferencer::commonType(type1, type2);
}

void BytecodeGenerator::addCast(VarType type, VarType targetType) {
    if (type == targetType) {
        return;
    }

    switch(targetType) {
    case VT_INT:
        if (type == VT_DOUBLE) {
            bytecode()->add(BC_D2I);
            return;
        }
        throw TranslationException("Bad cast from " + string(typeToName(type)) + " to " + string(typeToName(targetType)),
                                   currentPosition());
        break;
    case VT_DOUBLE:
        if (type == VT_INT) {
            bytecode()->add(BC_I2D);
            return;
        }
        throw TranslationException("Bad cast from " + string(typeToName(type)) + " to " + string(typeToName(targetType)),
                                   currentPosition());
        break;
    default:
        throw TranslationException("Bad cast from " + string(typeToName(type)) + " to " + string(typeToName(targetType)),
                                   currentPosition());
        break;
    }
}

void BytecodeGenerator::addIntDoubleInsn(VarType type, Instruction intInsn, Instruction doubleInsn) {
    checkNumber(type);
    switch(type) {
    case VT_INT:
        bytecode()->add(intInsn);
        break;
    case VT_DOUBLE:
        bytecode()->add(doubleInsn);
        break;
    default:
        break;
    }
}

void BytecodeGenerator::addStringIntDoubleInsn(VarType type, Instruction stringInsn, Instruction intInsn, Instruction doubleInsn) {
    switch(type) {
    case VT_STRING:
        bytecode()->add(stringInsn);
        break;
    case VT_INT:
        bytecode()->add(intInsn);
        break;
    case VT_DOUBLE:
        bytecode()->add(doubleInsn);
        break;
    default:
        throw TranslationException("Incorrect type: expected = \"string\" | \"double\" | \"int\", actual = " + string(typeToName(type)),
                                   currentPosition());
        break;
    }
}

void BytecodeGenerator::addAdd(VarType type) {
    addIntDoubleInsn(type, BC_IADD, BC_DADD);
}

void BytecodeGenerator::addSub(VarType type) {
    addIntDoubleInsn(type, BC_ISUB, BC_DSUB);
}

void BytecodeGenerator::addMul(VarType type) {
    addIntDoubleInsn(type, BC_IMUL, BC_DMUL);
}

void BytecodeGenerator::addDiv(VarType type) {
    addIntDoubleInsn(type, BC_IDIV, BC_DDIV);
}

void BytecodeGenerator::addMod(VarType type) {
    if (type == VT_INT) {
        bytecode()->add(BC_IMOD);
        return;
    }

    throw TranslationException("Incorrect type: expected = \"int\", actual = " + string(typeToName(type)),
                               currentPosition());
}

void BytecodeGenerator::addNeg(VarType type) {
    addIntDoubleInsn(type, BC_INEG, BC_DNEG);
}

/*
 * == 0 -> false
 * == 1 -> true
*/
void BytecodeGenerator::addCastIntToBool() {
    Label lJump(bytecode());
    Label lEnd(bytecode());

    bytecode()->add(BC_ILOAD0);
    bytecode()->addBranch(BC_IFICMPE, lJump);
    bytecode()->add(BC_ILOAD1);
    bytecode()->addBranch(BC_JA, lEnd);
    bytecode()->bind(lJump);
    bytecode()->add(BC_ILOAD0);
    bytecode()->bind(lEnd);
}

void BytecodeGenerator::addCastToBool(VarType type) {
    switch(type) {
    case VT_INT:
        addCastIntToBool();
        break;
    case VT_DOUBLE:
        bytecode()->add(BC_DLOAD0);
        bytecode()->add(BC_DCMP);
        addCastIntToBool();
        break;
    default:
        throw TranslationException("Bad cast from " + string(typeToName(type)) + " to bool",
                                   currentPosition());
        break;
    }
}

void BytecodeGenerator::addCompareWith0andReplace(Instruction ifInsn,
                                                  Instruction trueInsn,
                                                  Instruction falseInsn) {
    Label lTrue(bytecode());
    Label lEnd(bytecode());

    bytecode()->add(BC_ILOAD0);
    addSwap();
    bytecode()->addBranch(ifInsn, lTrue);
    bytecode()->add(falseInsn);
    bytecode()->addBranch(BC_JA, lEnd);
    bytecode()->bind(lTrue);
    bytecode()->add(trueInsn);
    bytecode()->bind(lEnd);
}

void BytecodeGenerator::addNot(VarType type) {
    addCastToBool(type);
    addCompareWith0andReplace(BC_IFICMPE, BC_ILOAD1, BC_ILOAD0);
}

void BytecodeGenerator::addCompInsn(VarType type, Instruction insn) {
    switch(type) {
    case VT_INT:
        bytecode()->add(BC_ICMP);
        break;
    case VT_DOUBLE:
        bytecode()->add(BC_DCMP);
        break;
    default:
        throw TranslationException("Incorrect type: expected = \"double\" | \"int\", actual = " + string(typeToName(type)),
                                   currentPosition());
        break;
    }

    addCompareWith0andReplace(insn, BC_ILOAD1, BC_ILOAD0);
}

void BytecodeGenerator::visitUnaryOpNode(UnaryOpNode *node) {
    refreshCurrentPosition(node);

    AstNode* operand = node->operand();
    operand->visit(this);
    VarType operandType = lastInferredType();

    TokenKind unOp = node->kind();
    switch(unOp) {
    case tADD:
        checkNumber(operandType);
        _lastType = operandType;
        break;
    case tSUB:
        addNeg(operandType);
        _lastType = operandType;
        break;
    case tNOT:
        addNot(operandType);
        _lastType = VT_INT;
        break;
    default:
        throw TranslationException("Incorrect token kind",
                                   currentPosition());
        break;
    }
}

void BytecodeGenerator::addBitOpInsn(VarType type, Instruction insn) {
    if (type == VT_INT) {
        bytecode()->add(insn);
        return;
    }
    throw TranslationException();
}

void BytecodeGenerator::addAAND(VarType type) {
    addBitOpInsn(type, BC_IAAND);
}

void BytecodeGenerator::addAOR(VarType type) {
    addBitOpInsn(type, BC_IAOR);
}

void BytecodeGenerator::addAXOR(VarType type) {
    addBitOpInsn(type, BC_IAXOR);
}

void BytecodeGenerator::addCastTwoLastToBool(VarType type) {
    addCastToBool(type);
    addSwap();
    addCastToBool(type);
    addSwap();
}

void BytecodeGenerator::visitBinaryOpNode(BinaryOpNode *node) {
    refreshCurrentPosition(node);

    AstNode* leftOperand = node->left();
    AstNode* rightOperand = node->right();
    TokenKind binOp = node->kind();

    leftOperand->visit(this);
    VarType leftType = lastInferredType();
    checkNumber(leftType);

    rightOperand->visit(this);
    VarType rightType = lastInferredType();
    checkNumber(rightType);

    VarType commonType = getCommonType(leftType, rightType);
    addSwap();
    addCast(leftType, commonType);
    addSwap();
    addCast(rightType, commonType);
    
    _lastType = commonType;

    switch (binOp) {
    //compare
    case tEQ:
        addSwap();
        addCompInsn(commonType, BC_IFICMPE);
        _lastType = VT_INT;
        break;
    case tNEQ:
        addSwap();
        addCompInsn(commonType, BC_IFICMPNE);
        _lastType = VT_INT;
        break;
    case tGE:
        addSwap();
        addCompInsn(commonType, BC_IFICMPGE);
        _lastType = VT_INT;
        break;
    case tLE:
        addSwap();
        addCompInsn(commonType, BC_IFICMPLE);
        _lastType = VT_INT;
        break;
    case tGT:
        addSwap();
        addCompInsn(commonType, BC_IFICMPG);
        _lastType = VT_INT;
        break;
    case tLT:
        addSwap();
        addCompInsn(commonType, BC_IFICMPL);
        _lastType = VT_INT;
        break;
        //logic
    case tOR:
        addCastTwoLastToBool(commonType);
        addAdd(commonType);
        _lastType = VT_INT;
        break;
    case tAND:
        addCastTwoLastToBool(commonType);
        addMul(commonType);
        _lastType = VT_INT;
        break;
        //bit
    case tAOR:
        addAOR(commonType);
        _lastType = VT_INT;
        break;
    case tAAND:
        addAAND(commonType);
        _lastType = VT_INT;
        break;
    case tAXOR:
        addAXOR(commonType);
        _lastType = VT_INT;
        break;
        //ariphm
    case tADD:
        addAdd(commonType);
        break;
    case tSUB:
        addSwap();
        addSub(commonType);
        break;
    case tMUL:
        addMul(commonType);
        break;
    case tDIV:
        addSwap();
        addDiv(commonType);
        break;
    case tMOD:
        addSwap();
        addMod(commonType);
        _lastType = VT_INT;
        break;
    case tRANGE:
        throw TranslationException("Improper use of RANGE",
                                   currentPosition());
        break;
    default:
        throw TranslationException("Incorrect token kind",
                                   currentPosition());
        break;
    }
}

void BytecodeGenerator::visitIntLiteralNode(IntLiteralNode *node) {
    refreshCurrentPosition(node);

    bytecode()->add(BC_ILOAD);
    bytecode()->addInt64(node->literal());

    _lastType = VT_INT;
}

void BytecodeGenerator::visitDoubleLiteralNode(DoubleLiteralNode *node) {
    refreshCurrentPosition(node);

    bytecode()->add(BC_DLOAD);
    bytecode()->addDouble(node->literal());

    _lastType = VT_DOUBLE;
}

uint16_t BytecodeGenerator::registerConstant(string const& constant) {
    return _code->makeStringConstant(constant);
}

void BytecodeGenerator::visitStringLiteralNode(StringLiteralNode *node) {
    refreshCurrentPosition(node);

    uint16_t id = registerConstant(node->literal());
    bytecode()->add(BC_SLOAD);
    bytecode()->addUInt16(id);

    _lastType = VT_STRING;
}

void BytecodeGenerator::loadValueFromVar(uint16_t scopeId,
                                         uint16_t varId,
                                         VarType varType) {
    switch(varType) {
    case VT_INT:
        bytecode()->add(BC_LOADCTXIVAR);
        break;
    case VT_DOUBLE:
        bytecode()->add(BC_LOADCTXDVAR);
        break;
    case VT_STRING:
        bytecode()->add(BC_LOADCTXSVAR);
        break;
    default:
        throw TranslationException("Incorrect type: expected = \"double\" | \"int\" | \"string\", actual = " + string(typeToName(varType)),
                                   currentPosition());
        break;
    }

    bytecode()->addUInt16(scopeId);
    bytecode()->addUInt16(varId);
}

void BytecodeGenerator::storeValueToVar(uint16_t scopeId,
                                        uint16_t varId,
                                        VarType varType) {
    switch(varType) {
    case VT_INT:
        bytecode()->add(BC_STORECTXIVAR);
        break;
    case VT_DOUBLE:
        bytecode()->add(BC_STORECTXDVAR);
        break;
    case VT_STRING:
        bytecode()->add(BC_STORECTXSVAR);
        break;
    default:
        throw TranslationException("Incorrect type: expected = \"double\" | \"int\" | \"string\", actual = " + string(typeToName(varType)),
                                   currentPosition());
        break;
    }

    bytecode()->addUInt16(scopeId);
    bytecode()->addUInt16(varId);
}

void BytecodeGenerator::visitStoreNode(StoreNode *node) {
    refreshCurrentPosition(node);

    TokenKind op = node->op();
    const AstVar* var = node->var();
    VarType varType = var->type();

    ScopeVarId scopeVarId = findScopeVarIdByName(var->name());

    uint16_t scopeId = scopeVarId.first;
    uint16_t varId = scopeVarId.second;

    AstNode* value = node->value();
    value->visit(this);
    VarType valueType = lastInferredType();
    addCast(valueType, varType);

    if (op == tINCRSET) {
        loadValueFromVar(scopeId, varId, varType);
        addAdd(varType);
    }
    else if (op == tDECRSET) {
        loadValueFromVar(scopeId, varId, varType);
        addSub(varType);
    }

    storeValueToVar(scopeId, varId, varType);
}

void BytecodeGenerator::visitLoadNode(LoadNode *node) {
    refreshCurrentPosition(node);

    const AstVar* var = node->var();
    ScopeVarId scopeVarId = findScopeVarIdByName(var->name());
    uint16_t scopeId = scopeVarId.first;
    uint16_t varId = scopeVarId.second;
    loadValueFromVar(scopeId, varId, var->type());

    _lastType = var->type();
}

void BytecodeGenerator::visitIfNode(IfNode *node) {
    refreshCurrentPosition(node);

    AstNode* ifExpr = node->ifExpr();
    ifExpr->visit(this);
    VarType ifExprType = lastInferredType();
    addCastToBool(ifExprType);

    Label lElse(bytecode());
    Label lEndIf(bytecode());

    bytecode()->add(BC_ILOAD0);
    bytecode()->addBranch(BC_IFICMPE, lElse);
    BlockNode* thenBlock = node->thenBlock();
    thenBlock->visit(this);
    bytecode()->addBranch(BC_JA, lEndIf);

    bytecode()->bind(lElse);
    BlockNode* elseBlock = node->elseBlock();
    if (elseBlock) {
        elseBlock->visit(this);
    }
    bytecode()->bind(lEndIf);
}

void BytecodeGenerator::visitWhileNode(WhileNode *node) {
    refreshCurrentPosition(node);

    AstNode* whileExpr = node->whileExpr();
    BlockNode* whileBlock = node->loopBlock();

    Label lBegin(bytecode());
    Label lEnd(bytecode());

    bytecode()->bind(lBegin);
    whileExpr->visit(this);
    bytecode()->add(BC_ILOAD0);
    bytecode()->addBranch(BC_IFICMPE, lEnd);

    whileBlock->visit(this);

    bytecode()->addBranch(BC_JA, lBegin);
    bytecode()->bind(lEnd);
}

void BytecodeGenerator::visitForNode(ForNode *node) {
    refreshCurrentPosition(node);

    const AstVar* inVar = node->var();
    VarType inVarType = inVar->type();
    if (inVarType != VT_INT) {
        throw TranslationException("Incorrect type: expected = \"int\", actual = " + string(typeToName(inVarType)),
                                   currentPosition());
    }
    string inVarName = inVar->name();

    AstNode* inExpr = node->inExpr();
    if (!inExpr->isBinaryOpNode() || inExpr->asBinaryOpNode()->kind() != tRANGE) {
        throw TranslationException("Incorrect \"FOR\" expression",
                                   currentPosition());
    }
    AstNode* left = inExpr->asBinaryOpNode()->left();
    left->visit(this);
    addCast(lastInferredType(), inVarType);

    AstNode* right = inExpr->asBinaryOpNode()->right();
    right->visit(this);
    addCast(lastInferredType(), inVarType);
    addSwap();

    BlockNode* forBlock = node->body();
    uint16_t blockScopeId = currentFunctionTranslationContext()->getScopeId();

    ScopeVarId inScopeVarId = findScopeVarIdByName(inVarName);
    uint16_t scopeId = inScopeVarId.first;
    uint16_t inVarId = inScopeVarId.second;

    uint16_t leftRangeId, rightRangeId;
    try {
        leftRangeId = currentFunctionTranslationContext()->addVar("<left>");
        rightRangeId = currentFunctionTranslationContext()->addVar("<right>");
    }
    catch (TranslationException const& exception) {
        throw TranslationException(exception.what(), currentPosition());
    }

    storeValueToVar(blockScopeId, leftRangeId, inVarType);
    storeValueToVar(blockScopeId, rightRangeId, inVarType);

    Label forBegin(bytecode());
    Label forEnd(bytecode());

    forBegin.bind(bytecode()->length());

    loadValueFromVar(blockScopeId, leftRangeId, inVarType);
    storeValueToVar(scopeId, inVarId, inVarType);

    loadValueFromVar(blockScopeId, rightRangeId, inVarType);
    loadValueFromVar(scopeId, inVarId, inVarType);

    bytecode()->addBranch(BC_IFICMPG, forEnd);

    forBlock->visit(this);
    loadValueFromVar(blockScopeId, leftRangeId, inVarType);
    bytecode()->add(BC_ILOAD1);
    bytecode()->add(BC_IADD);
    storeValueToVar(blockScopeId, leftRangeId, inVarType);

    bytecode()->addBranch(BC_JA, forBegin);

    forEnd.bind(bytecode()->length());

    currentFunctionTranslationContext()->removeVarId("<left>");
    currentFunctionTranslationContext()->removeVarId("<right>");
}

void BytecodeGenerator::addReturn() {
    bytecode()->add(BC_RETURN);
}

void BytecodeGenerator::addSwap() {
    bytecode()->add(BC_SWAP);
}

void BytecodeGenerator::visitReturnNode(ReturnNode *node) {
    refreshCurrentPosition(node);

    uint16_t id = currentFunctionTranslationContext()->getScopeId();
    BytecodeFunction* bcFunction = dynamic_cast<BytecodeFunction*>(_code->functionById(id));
    VarType targetReturnType = bcFunction->returnType();

    AstNode* returnExpr = node->returnExpr();
    if (returnExpr != NULL) {
        returnExpr->visit(this);
        VarType returnExprType = lastInferredType();
        addCast(returnExprType, targetReturnType);
    }
    addReturn();
}

void BytecodeGenerator::addPrint(VarType type) {
    switch(type) {
    case VT_INT:
        bytecode()->add(BC_IPRINT);
        break;
    case VT_DOUBLE:
        bytecode()->add(BC_DPRINT);
        break;
    case VT_STRING:
        bytecode()->add(BC_SPRINT);
        break;
    default:
        throw TranslationException("Incorrect type: expected = \"double\" | \"int\" | \"string\", actual = " + string(typeToName(type)),
                                   currentPosition());
        break;
    }
}

void BytecodeGenerator::visitPrintNode(PrintNode *node) {
    refreshCurrentPosition(node);

    for (uint32_t i = 0; i < node->operands(); ++i) {
        AstNode* operand = node->operandAt(i);
        operand->visit(this);
        VarType operandType = lastInferredType();
        addPrint(operandType);
    }
}

void BytecodeGenerator::visitNativeCallNode(NativeCallNode *node) {
    refreshCurrentPosition(node);
}

void BytecodeGenerator::visitCallNode(CallNode *node) {
    refreshCurrentPosition(node);

    uint16_t calleeId;
    if (isNative(node->name())) {
        calleeId = getNativeIdByName(node->name());
        const Signature* signature;
        const string* name;
        _code->nativeById(calleeId, &signature, &name);

        assert(signature->size() > 0);
        for (int i = signature->size() - 1; i > 0; --i) {
            AstNode* arg = node->parameterAt(i);
            arg->visit(this);
            VarType argType = lastInferredType();
            VarType paramType = signature->at(i).first;
            addCast(argType, paramType);
        }

        bytecode()->add(BC_CALLNATIVE);

        _lastType = signature->at(0).first;
    }
    else {
        calleeId = getFunctionIdByName(node->name());
        BytecodeFunction* bcFunction = dynamic_cast<BytecodeFunction*>(_code->functionById(calleeId));//_bcFunctions.at(calleeId);

        for (int i = node->parametersNumber() - 1; i > -1; --i) {
            AstNode* arg = node->parameterAt(i);
            arg->visit(this);
            VarType argType = lastInferredType();
            VarType paramType = bcFunction->parameterType(i);
            addCast(argType, paramType);
        }

        bytecode()->add(BC_CALL);

        _lastType = bcFunction->returnType();
    }

    bytecode()->addInt16(calleeId);
}

void BytecodeGenerator::visitBlockNode(BlockNode *node) {
    refreshCurrentPosition(node);

    Scope* scope = node->scope();
    FunctionTranslationContext* context = currentFunctionTranslationContext();
    try {
        context->registerScopeVars(scope);
    }
    catch (TranslationException const& exception) {
        throw TranslationException(exception.what(), currentPosition());
    }

    registerScopeFunctions(scope);

    for (size_t i = 0; i < node->nodes(); ++i) {
        node->nodeAt(i)->visit(this);
    }

    translateScopeFunctions(scope);

    context->unregisterScopeVars(scope);
}

void BytecodeGenerator::registerScopeFunctions(Scope* scope) {
    Scope::FunctionIterator funcIt(scope);
    while(funcIt.hasNext()) {
        AstFunction* function = funcIt.next();
        if (function->node()->body()->nodeAt(0)->isNativeCallNode()) {
            registerNativeFunction(function->node()->body()->nodeAt(0)->asNativeCallNode());
        }
        else {
            registerFunction(function);
        }
    }

}

void BytecodeGenerator::registerNativeFunction(NativeCallNode* native) {
    uint16_t nativeId = _code->makeNativeFunction(native->nativeName(), native->nativeSignature(), NULL);
    _nativeIdByName[native->nativeName()] = nativeId;
}

void BytecodeGenerator::registerFunction(AstFunction *astFunction) {
    assert(_functionIdByName.find(astFunction->name()) == _functionIdByName.end());

    BytecodeFunction* bcFunction = new BytecodeFunction(astFunction);
    uint16_t bcFunctionId = _code->addFunction(bcFunction);

    _functionIdByName[bcFunction->name()] = bcFunctionId;
}

void BytecodeGenerator::translateScopeFunctions(Scope *scope) {
    Scope::FunctionIterator funcIt(scope);
    while(funcIt.hasNext()) {
        AstFunction* function = funcIt.next();
        function->node()->visit(this);
    }
}

bool BytecodeGenerator::isNative(const string &name) {
    return _nativeIdByName.find(name) != _nativeIdByName.end();
}

uint16_t BytecodeGenerator::getNativeIdByName(const string &name) {
    assert(_nativeIdByName.find(name) != _nativeIdByName.end());
    return _nativeIdByName[name];
}

uint16_t BytecodeGenerator::getFunctionIdByName(const string &name) {
    assert(_functionIdByName.find(name) != _functionIdByName.end());
    return _functionIdByName[name];
}

/*
 * This FunctionNode contain really function, not native
*/
void BytecodeGenerator::collectArgs(FunctionNode *node) {
    for (size_t i = 0; i < node->parametersNumber(); ++i) {
        ScopeVarId scopeVarId = findScopeVarIdByName(node->parameterName(i));
        VarType type = node->parameterType(i);
        addStringIntDoubleInsn(type, BC_STORECTXSVAR, BC_STORECTXIVAR, BC_STORECTXDVAR);
        bytecode()->addUInt16(scopeVarId.first);
        bytecode()->addUInt16(scopeVarId.second);
    }
}

/*
 * Start translation of function with specified unique name
 * Function with specified name (unique!) must be already registered
 * can contain only NativeCallNode
*/
void BytecodeGenerator::visitFunctionNode(FunctionNode *node) {
    refreshCurrentPosition(node);

    assert(_functionIdByName.find(node->name()) != _functionIdByName.end());

    if (isNative(node->name())) {
        return;
    }

    uint16_t functionId = _functionIdByName[node->name()];
    BytecodeFunction* bcFunction = dynamic_cast<BytecodeFunction*>(_code->functionById(functionId)); //_bcFunctions[functionId];
    bcFunction->setScopeId(functionId);
    _bytecodeStack.push_back(bcFunction->bytecode());

    FunctionTranslationContext* ftcontext = addNewFunctionTranslationContext(functionId);
    ftcontext->registerSignature(node->signature());

    collectArgs(node);
    node->body()->visit(this);

    ftcontext->unregisterSignature(node->signature());
    removeLastFunctionTranslationContext();
    _bytecodeStack.pop_back();
}

FunctionTranslationContext* BytecodeGenerator::addNewFunctionTranslationContext(uint16_t functionScopeId) {
    FunctionTranslationContext* context = new FunctionTranslationContext(functionScopeId);
    _contexts.push_back(context);

    return context;
}

void BytecodeGenerator::removeLastFunctionTranslationContext() {
    FunctionTranslationContext* context = _contexts.back();
    delete context;
    _contexts.pop_back();
}

FunctionTranslationContext* BytecodeGenerator::currentFunctionTranslationContext() {
    return _contexts.back();
}

ScopeVarId BytecodeGenerator::findScopeVarIdByName(const string &name) {
    for (std::vector<FunctionTranslationContext*>::reverse_iterator contextRevIt = _contexts.rbegin(); contextRevIt != _contexts.rend(); ++contextRevIt) {
        FunctionTranslationContext* context = *contextRevIt;
        if (context->varNameExist(name)) {
            return context->getScopeVarId(name);
        }
    }
    throw TranslationException("Var with name = \"" + name + "\" is not defined",
                               currentPosition());
}

void BytecodeGenerator::addCallTopFunction() {
    BytecodeFunction* top = dynamic_cast<BytecodeFunction*>(_code->functionByName("<top>"));
    top->bytecode()->add(BC_CALL);

    assert(top->id() == 0);
    top->bytecode()->addUInt16(top->id());
}

Status* BytecodeGenerator::makeBytecode(AstFunction* top, InterpreterCodeImpl* *code) {
    _code = new InterpreterCodeImpl();
    try {
        registerFunction(top);
        top->node()->visit(this);
    }
    catch(TranslationException const& exception) {
        return Status::Error(exception.what(), exception.position());
    }

    *code = _code;
    return Status::Ok();
}

}
