#include "bytecode_generator.h"
#include "interpreter_code_impl.h"
#include "my_utils.h"

#include <memory>

using std::shared_ptr;

namespace mathvm {

void BytecodeGenerator::visitProgram(AstFunction* astFun) {
    visitFuncNodeWithInit(astFun);
}

void BytecodeGenerator::visitFuncNodeWithInit(AstFunction* astFun) {
    BytecodeFunction* translatedFun = createBytecodeFun(astFun);
    shared_ptr<Context> ctx = createContextWithArgs(astFun->node(), translatedFun->id());
    genArgsStoreBc(translatedFun->bytecode(), astFun->node());

    _funIdsStack.push_back(translatedFun->id());
    astFun->node()->visit(this);
    _funIdsStack.pop_back();

    translatedFun->setLocalsNumber(ctx->varsNumber() - translatedFun->parametersNumber());
}

void BytecodeGenerator::visitFunctionNode(FunctionNode* node) {
    DEBUG_MSG("visitFunctionNode: " + currentBcFunction()->name());
    if(node->body()->nodeAt(0)->isNativeCallNode()) {
        node->body()->nodeAt(0)->asNativeCallNode()->visit(this);
    } else {
        node->body()->visit(this);
    }
}

void BytecodeGenerator::visitNativeCallNode(NativeCallNode *node) {
    // not implemented
}

void BytecodeGenerator::visitBlockNode(BlockNode* node) {
    visitVarDecls(node);
    visitFunDefs(node);
    visitExprs(node);
}

void BytecodeGenerator::visitUnaryOpNode(UnaryOpNode* node) {
    DEBUG_MSG("visitUnaryOpNode");
    node->operand()->visit(this);
    Bytecode* bc = currentBcToFill();
    if(node->kind() == tSUB) {
        MaybeInsn mbNeg = typedInsnNumericsOnly(_typesStack.back(), BC_INEG, BC_DNEG);
        if(!mbNeg.first) {
            throw TranslatorException(wrongTypeMsg(), node->operand()->position());
        }
        bc->addInsn(mbNeg.second);
    } else if (node->kind() == tNOT) {
        genNotBc(bc, node);
        _typesStack.pop_back();
        _typesStack.push_back(VT_INT);
    } else if (node->kind() == tADD) {
        // unary '+' has no effect
    } else {
        throw TranslatorException(invalidUnaryOperatorMsg(node->kind()), node->position());
    }
}

void BytecodeGenerator::visitBinaryOpNode(BinaryOpNode* node) {
    node->right()->visit(this);
    node->left()->visit(this);
    VarType rightOpType = _typesStack[_typesStack.size() - 2];
    VarType leftOpType = _typesStack.back();
    _typesStack.pop_back();
    _typesStack.pop_back();
    throwIfTypesIncompatible(node->position(), leftOpType, rightOpType);
    Bytecode* bc = currentBcToFill();
    if(leftOpType == VT_STRING) {
        handleStrBinOps(node, bc);
    } else {
        makeTypesSameIfNeeded(bc, leftOpType, rightOpType);
        bool handled = handleBaseArithmOps(leftOpType, node, bc) ||
                       handleIntArithmOps(leftOpType, node, bc) ||
                       handleIntCompOps(leftOpType, node, bc) ||
                       handleDoubleCompOps(leftOpType, node, bc) ||
                       handleLogicOps(leftOpType, node, bc) ||
                       handleRangeOp(leftOpType, node, bc);
        if(!handled) {
            throw TranslatorException(invalidBinOpMsg(node->kind()), node->position());
        }
    }
}

void BytecodeGenerator::visitLoadNode(LoadNode* node) {
    DEBUG_MSG("visitLoadNode: " + node->var()->name());
    string const& varName = node->var()->name();
    VarType varType = node->var()->type();
    _typesStack.push_back(varType);
    Bytecode* bc = currentBcToFill();

    shared_ptr<Context> curCtx = currentCtx();
    if(curCtx->hasVar(varName)) {
        MaybeInsn mbInsn = typedInsn(varType, BC_LOADIVAR, BC_LOADDVAR, BC_LOADSVAR);
        if(!mbInsn.first) {
            throw TranslatorException(wrongTypeMsg(), node->position());
        }
        genBcInsnWithId(bc, mbInsn.second, curCtx->getVarId(varName));
    } else {
        shared_ptr<Context> outerCtx = findVarInOuterCtx(varName);
        if(!outerCtx) {
            throw TranslatorException(varNotDeclaredMsg(varName), node->position());
        }
        MaybeInsn mbInsn = typedInsn(varType, BC_LOADCTXIVAR, BC_LOADCTXDVAR, BC_LOADCTXSVAR);
        if(!mbInsn.first) {
            throw TranslatorException(wrongTypeMsg(), node->position());
        }
        genBcInsnWithTwoIds(bc, mbInsn.second, outerCtx->id(), outerCtx->getVarId(varName));
    }
}

void BytecodeGenerator::visitStoreNode(StoreNode* node) {
    DEBUG_MSG("visitStoreNode: " + node->var()->name());
    string const& varName = node->var()->name();
    VarType varType = node->var()->type();

    node->value()->visit(this);
    VarType valueType = _typesStack.back();
    _typesStack.pop_back();

    Bytecode* bc = currentBcToFill();
    throwIfTypesIncompatible(node->position(), varType, valueType);
    castIfNeeded(valueType, node, bc, varType);

    shared_ptr<Context> curCtx = currentCtx();
    if(curCtx->hasVar(varName)) {
        uint16_t varId = curCtx->getVarId(varName);
        if(node->op() != tASSIGN) {
            if(varType == VT_STRING) {
                throw TranslatorException(invalidStrOperationMsg(), node->position());
            }
            Instruction bcLoadvar = typedInsnNumericsOnly(varType, BC_LOADIVAR, BC_LOADDVAR).second;
            genBcInsnWithId(bc, bcLoadvar, varId);
            genAssignOpBc(node, bc, varType);
        }
        Instruction bcStore = typedInsn(varType, BC_STOREIVAR, BC_STOREDVAR, BC_STORESVAR).second;
        genBcInsnWithId(bc, bcStore, varId);
    } else {
        shared_ptr<Context> outerCtx = findVarInOuterCtx(varName);
        if(!outerCtx) {
            throw TranslatorException(varNotDeclaredMsg(varName), node->position());
        }
        uint16_t varId = outerCtx->getVarId(varName);
        if(node->op() != tASSIGN) {
            if(varType == VT_STRING) {
                throw TranslatorException(invalidStrOperationMsg(), node->position());
            }
            Instruction bcLoadctxvar = typedInsnNumericsOnly(varType, BC_LOADCTXIVAR, BC_LOADCTXDVAR).second;
            genBcInsnWithTwoIds(bc, bcLoadctxvar, outerCtx->id(), varId);
            genAssignOpBc(node, bc, varType);
        }
        Instruction bcStorectx = typedInsn(varType, BC_STORECTXIVAR, BC_STORECTXDVAR, BC_STORECTXSVAR).second;
        genBcInsnWithTwoIds(bc, bcStorectx, outerCtx->id(), varId);
    }
    _typesStack.push_back(varType);
}

void BytecodeGenerator::visitIntLiteralNode(IntLiteralNode* node) {
    Bytecode* bc = currentBcToFill();
    bc->addInsn(BC_ILOAD);
    bc->addInt64(node->literal());
    _typesStack.push_back(VT_INT);
}

void BytecodeGenerator::visitDoubleLiteralNode(DoubleLiteralNode* node) {
    Bytecode* bc = currentBcToFill();
    bc->addInsn(BC_DLOAD);
    bc->addDouble(node->literal());
    _typesStack.push_back(VT_DOUBLE);
}

void BytecodeGenerator::visitStringLiteralNode(StringLiteralNode* node) {
    uint16_t stringId = _code->makeStringConstant(node->literal());
    Bytecode* bc = currentBcToFill();
    genBcInsnWithId(bc, BC_SLOAD, stringId);
    _typesStack.push_back(VT_STRING);
}

void BytecodeGenerator::visitPrintNode(PrintNode* node) {
    Bytecode* bc = currentBcToFill();
    for(size_t i = 0; i != node->operands(); ++i) {
        node->operandAt(i)->visit(this);
        MaybeInsn mbInsn = typedInsn(_typesStack.back(), BC_IPRINT, BC_DPRINT, BC_SPRINT);
        if(mbInsn.first) {
            bc->addInsn(mbInsn.second);
            _typesStack.pop_back();
        } else {
            throw TranslatorException(wrongTypeMsg(), node->operandAt(i)->position());
        }
    }
    _typesStack.push_back(VT_VOID);
}

void BytecodeGenerator::visitCallNode(CallNode* node) {
    DEBUG_MSG("visitCallNode: " + node->name());
    for(size_t i = node->parametersNumber(); i > 0; --i) {
        node->parameterAt(i - 1)->visit(this);
    }
    TranslatedFunction* funPtr = _code->functionByName(node->name());
    if(funPtr) {
        Bytecode* bc = currentBcToFill();
        bc->addInsn(BC_CALL);
        bc->addInt16(funPtr->id());
    } else {
        throw TranslatorException(undefFunMsg(node->name()), node->position());
    }
}

void BytecodeGenerator::visitReturnNode(ReturnNode* node) {
    DEBUG_MSG("visitReturnNode");
    if(node->returnExpr() != 0) {
        node->returnExpr()->visit(this);
    }
    Bytecode* bc = currentBcToFill();
    bc->addInsn(BC_RETURN);
}

void BytecodeGenerator::visitIfNode(IfNode* node) {
    DEBUG_MSG("visitIfNode");
    Bytecode* bc = currentBcToFill();
    node->ifExpr()->visit(this);
    if(_typesStack.back() != VT_INT) {
        throw TranslatorException(wrongTypeMsg(VT_INT), node->ifExpr()->position());
    }
    bc->addInsn(BC_ILOAD0);
    bc->addInsn(BC_ICMP);
    Label labelElse(bc);
    bc->addBranch(BC_IFICMPE, labelElse);
    const size_t COND_CHECK_RESULTS_ON_STACK = 3;
    genInsnNTimes(bc, BC_POP, COND_CHECK_RESULTS_ON_STACK);
    node->thenBlock()->visit(this);
    Label labelAfterElse(bc);
    bc->addBranch(BC_JA, labelAfterElse);
    bc->bind(labelElse);
    genInsnNTimes(bc, BC_POP, COND_CHECK_RESULTS_ON_STACK);
    if(node->elseBlock() != 0) {
        node->elseBlock()->visit(this);
    }
    bc->bind(labelAfterElse);
}

void BytecodeGenerator::visitWhileNode(WhileNode* node) {
    DEBUG_MSG("visitWhileNode");
    Bytecode* bc = currentBcToFill();
    Label whileStart(bc);
    bc->bind(whileStart);
    node->whileExpr()->visit(this);
    if(_typesStack.back() != VT_INT) {
        throw TranslatorException(wrongTypeMsg(VT_INT), node->whileExpr()->position());
    }
    bc->addInsn(BC_ILOAD0);
    bc->addInsn(BC_ICMP);
    Label whileEnd(bc);
    bc->addBranch(BC_IFICMPE, whileEnd);
    const size_t COND_CHECK_RESULTS_ON_STACK = 3;
    genInsnNTimes(bc, BC_POP, COND_CHECK_RESULTS_ON_STACK);
    node->loopBlock()->visit(this);
    bc->addBranch(BC_JA, whileStart);
    bc->bind(whileEnd);
    genInsnNTimes(bc, BC_POP, COND_CHECK_RESULTS_ON_STACK);
}

void BytecodeGenerator::visitForNode(ForNode* node) {
    DEBUG_MSG("visitForNode: " + node->var()->name());
    shared_ptr<Context> ctx = currentCtx();
    string const& iterVarName = node->var()->name();
    if(!ctx->hasVar(iterVarName)) {
        throw TranslatorException(varNotDeclaredMsg(iterVarName), node->position());
    }
    uint16_t iterVarId = ctx->getVarId(iterVarName);
    uint16_t upperVarId = ctx->addVar("<upper>");
    Bytecode* bc = currentBcToFill();

    node->inExpr()->visit(this);
    VarType rangeOpType = _typesStack.back();
    _typesStack.pop_back();

    if(rangeOpType == VT_INT) {
        genForNodeBc(bc, node, iterVarId, upperVarId,
                     BC_STOREIVAR, BC_LOADIVAR, BC_ICMP, BC_ILOAD1, BC_IADD);
    } else {
        genForNodeBc(bc, node, iterVarId, upperVarId,
                     BC_STOREDVAR, BC_LOADDVAR, BC_DCMP, BC_DLOAD1, BC_DADD);
    }
}

// private methods section

void BytecodeGenerator::visitVarDecls(BlockNode* node) {
    Bytecode* bc = currentBcToFill();
    shared_ptr<Context> ctx = currentCtx();

    Scope::VarIterator varIt(node->scope());
    while(varIt.hasNext()) {
        AstVar* var = varIt.next();
        uint16_t varId = ctx->addVar(var->name());
        if(!genVarDeclBc(bc, var, varId)) {
            throw TranslatorException(wrongVarDeclMsg(currentBcFunction()->name(), var->name()), 0);
        }
    }
}

void BytecodeGenerator::visitFunDefs(BlockNode* node) {
    Scope::FunctionIterator funIt(node->scope());
    while(funIt.hasNext()) {
        AstFunction* fun = funIt.next();
        visitFuncNodeWithInit(fun);
    }
}

void BytecodeGenerator::visitExprs(BlockNode* node) {
    DEBUG_MSG("visitExprs");
    for(size_t i = 0; i != node->nodes(); ++i) {
        node->nodeAt(i)->visit(this);
    }
}

BytecodeFunction* BytecodeGenerator::createBytecodeFun(AstFunction* astFun) {
    BytecodeFunction* bcFun = new BytecodeFunction(astFun);
    uint16_t funId = _code->addFunction(bcFun);
    bcFun->setScopeId(funId);
    return bcFun;
}

shared_ptr<Context> BytecodeGenerator::createContextWithArgs(FunctionNode* fNode, uint16_t transFunId) {
    shared_ptr<Context> newCtx;
    if(_funIdsStack.empty()) {
        newCtx = _code->createTopmostCtx(transFunId);
    } else {
        newCtx = _code->createCtx(transFunId, _funIdsStack.back());
    }
    for(size_t i = 0; i != fNode->parametersNumber(); ++i) {
        newCtx->addVar(fNode->parameterName(i));
        _typesStack.pop_back();
    }
    return newCtx;
}

shared_ptr<Context> BytecodeGenerator::findVarInOuterCtx(string const& name) {
    if(_funIdsStack.size() > 1) {
        for(size_t i = _funIdsStack.size() - 2; i >= 0; --i) {
            shared_ptr<Context> outerCtx = _code->ctxById(i);
            if(outerCtx->hasVar(name)) {
                return outerCtx;
            }
        }
    }
    return shared_ptr<Context>();
}

bool BytecodeGenerator::genVarDeclBc(Bytecode* bc, AstVar* var, uint16_t varId) {
    MaybeInsn mbLoad0Insn = typedInsn(var->type(), BC_ILOAD0, BC_DLOAD0, BC_SLOAD0);
    if(mbLoad0Insn.first) {
        bc->addInsn(mbLoad0Insn.second);
        MaybeInsn mbStoreInsn = typedInsn(var->type(), BC_STOREIVAR, BC_STOREDVAR, BC_STORESVAR);
        genBcInsnWithId(bc, mbStoreInsn.second, varId);
        return true;
    }
    return false;
}

void BytecodeGenerator::genArgsStoreBc(Bytecode* bc, FunctionNode* node) {
    for(uint16_t i = 0; i != node->parametersNumber(); ++i) {
        MaybeInsn mbInsn = typedInsn(node->parameterType(i), BC_STOREIVAR, BC_STOREDVAR, BC_STORESVAR);
        if(mbInsn.first) {
            genBcInsnWithId(bc, mbInsn.second, i);
        } else {
            throw TranslatorException(wrongParameterTypeMsg(node->name(), i), node->position());
        }
    }
}

void BytecodeGenerator::genForNodeBc(Bytecode* bc, ForNode* node,
                                     uint16_t iterVarId, uint16_t upperVarId,
                                     Instruction bcStoreVar, Instruction bcLoadVar,
                                     Instruction bcCmp, Instruction bcLoad1, Instruction bcAdd) {
    vector<uint16_t> varsIds(2);
    varsIds[0] = iterVarId;
    varsIds[1] = upperVarId;
    // init iter var and upper bound var
    genTransferDataBc(bc, bcStoreVar, varsIds);
    // begin for: check iter_var <= upper_var
    Label forStart(bc);
    bc->bind(forStart);
    genTransferDataBc(bc, bcLoadVar, varsIds);
    bc->addInsn(bcCmp);
    bc->addInsn(BC_ILOAD1);
    Label forEnd(bc);
    bc->addBranch(BC_IFICMPE, forEnd);
    // then
    const size_t RESULTS_ON_STACK = 4;
    genInsnNTimes(bc, BC_POP, RESULTS_ON_STACK);
    node->body()->visit(this);
    // ++ iter_var
    genIncrementBc(bc, iterVarId, bcLoadVar, bcLoad1, bcAdd, bcStoreVar);
    bc->addBranch(BC_JA, forStart);
    bc->bind(forEnd);
    genInsnNTimes(bc, BC_POP, RESULTS_ON_STACK);
}

void BytecodeGenerator::genNotBc(Bytecode* bc, UnaryOpNode* node) {
    MaybeInsn mbLoad0 = typedInsnNumericsOnly(_typesStack.back(), BC_ILOAD0, BC_DLOAD0);
    if(!mbLoad0.first) {
        throw TranslatorException(wrongTypeMsg(), node->operand()->position());
    }
    MaybeInsn mbCmp = typedInsnNumericsOnly(_typesStack.back(), BC_ICMP, BC_DCMP);
    bc->addInsn(mbLoad0.second);
    bc->addInsn(mbCmp.second);
    bc->addInsn(BC_ILOAD0);
    bc->addInsn(BC_ICMP);
    const size_t COND_CHECK_RESULTS_ON_STACK = 5;
    genBoolFromIficmp(bc, BC_IFICMPE, COND_CHECK_RESULTS_ON_STACK);
}

void BytecodeGenerator::throwIfTypesIncompatible(size_t nodePos, VarType leftOpType, VarType rightOpType) {
    if(leftOpType == VT_VOID || rightOpType == VT_VOID) {
        throw TranslatorException(wrongTypeMsg(), nodePos);
    }
    if(leftOpType == VT_INVALID || rightOpType == VT_INVALID) {
        throw TranslatorException(invalidTypeMsg(), nodePos);
    }
    if((leftOpType == VT_STRING && rightOpType != VT_STRING) ||
       (leftOpType != VT_STRING && rightOpType == VT_STRING)) {
        throw TranslatorException(binOpWithStrAndNonStrMsg(), nodePos);
    }
}

void BytecodeGenerator::handleStrBinOps(BinaryOpNode* node, Bytecode* bc) {
    const size_t OPERANDS_ON_STACK = 2;
    if(node->kind() == tEQ) {
        genBoolFromIficmp(bc, BC_IFICMPE, OPERANDS_ON_STACK);
    } else if(node->kind() == tNEQ) {
        genBoolFromIficmp(bc, BC_IFICMPNE, OPERANDS_ON_STACK);
    } else {
        throw TranslatorException(invalidStrOperationMsg(), node->position());
    }
    _typesStack.push_back(VT_INT);
}

void BytecodeGenerator::makeTypesSameIfNeeded(Bytecode* bc, VarType& leftOpType, VarType& rightOpType) {
    if(leftOpType == VT_INT && rightOpType == VT_DOUBLE) {
        bc->addInsn(BC_SWAP);
        bc->addInsn(BC_I2D);
        bc->addInsn(BC_SWAP);
        leftOpType == VT_DOUBLE;
    } else if(leftOpType == VT_DOUBLE && rightOpType == VT_INT) {
        bc->addInsn(BC_I2D);
        rightOpType = VT_DOUBLE;
    }
}

bool BytecodeGenerator::handleBaseArithmOps(VarType leftOpType, BinaryOpNode* node, Bytecode* bc) {
    if(node->kind() == tADD) {
        MaybeInsn mbAdd = typedInsnNumericsOnly(leftOpType, BC_IADD, BC_DADD);
        bc->addInsn(mbAdd.second);
    } else if(node->kind() == tSUB) {
        MaybeInsn mbSub = typedInsnNumericsOnly(leftOpType, BC_ISUB, BC_DSUB);
        bc->addInsn(mbSub.second);
    } else if(node->kind() == tMUL) {
        MaybeInsn mbMul = typedInsnNumericsOnly(leftOpType, BC_IMUL, BC_DMUL);
        bc->addInsn(mbMul.second);
    } else if(node->kind() == tDIV) {
        MaybeInsn mbDiv = typedInsnNumericsOnly(leftOpType, BC_IDIV, BC_DDIV);
        bc->addInsn(mbDiv.second);
    } else {
        return false;
    }
    _typesStack.push_back(leftOpType);
    return true;
}

bool BytecodeGenerator::handleIntArithmOps(VarType leftOpType, BinaryOpNode* node, Bytecode* bc) {
    MaybeInsn mbOp(false, BC_INVALID);
    if(node->kind() == tMOD) {
        mbOp = MaybeInsn(true, BC_IMOD);
    } else if(node->kind() == tAAND) {
        mbOp = MaybeInsn(true, BC_IAAND);
    } else if(node->kind() == tAOR) {
        mbOp = MaybeInsn(true, BC_IAOR);
    } else if(node->kind() == tAXOR) {
        mbOp = MaybeInsn(true, BC_IAXOR);
    }
    if(!mbOp.first) {
        return false;
    }
    if(leftOpType != VT_INT) {
        throw TranslatorException(wrongTypeMsg(), node->position());
    }
    bc->addInsn(mbOp.second);
    _typesStack.push_back(VT_INT);
    return true;
}

bool BytecodeGenerator::handleIntCompOps(VarType leftOpType, BinaryOpNode* node, Bytecode* bc) {
    if(leftOpType != VT_INT) {
        return false;
    }
    const size_t OPERANDS_ON_STACK = 2;
    if(node->kind() == tEQ) {
        genBoolFromIficmp(bc, BC_IFICMPE, OPERANDS_ON_STACK);
    } else if(node->kind() == tNEQ) {
        genBoolFromIficmp(bc, BC_IFICMPNE, OPERANDS_ON_STACK);
    } else if(node->kind() == tGT) {
        genBoolFromIficmp(bc, BC_IFICMPG, OPERANDS_ON_STACK);
    } else if(node->kind() == tGE) {
        genBoolFromIficmp(bc, BC_IFICMPGE, OPERANDS_ON_STACK);
    } else if(node->kind() == tLT) {
        genBoolFromIficmp(bc, BC_IFICMPL, OPERANDS_ON_STACK);
    } else if(node->kind() == tLE) {
        genBoolFromIficmp(bc, BC_IFICMPLE, OPERANDS_ON_STACK);
    }
    _typesStack.push_back(VT_INT);
    return true;
}

bool BytecodeGenerator::handleDoubleCompOps(VarType leftOpType, BinaryOpNode* node, Bytecode* bc) {
    if(leftOpType != VT_DOUBLE) {
        return false;
    }
    bc->addInsn(BC_DCMP);
    bc->addInsn(BC_DLOAD0);
    const size_t OPERANDS_ON_STACK = 4;
    if(node->kind() == tEQ) {
        genBoolFromIficmp(bc, BC_IFICMPE, OPERANDS_ON_STACK);
    } else if(node->kind() == tNEQ) {
        genBoolFromIficmp(bc, BC_IFICMPNE, OPERANDS_ON_STACK);
    } else if(node->kind() == tGT) {
        genBoolFromIficmp(bc, BC_IFICMPL, OPERANDS_ON_STACK);
    } else if(node->kind() == tGE) {
        genBoolFromIficmp(bc, BC_IFICMPLE, OPERANDS_ON_STACK);
    } else if(node->kind() == tLT) {
        genBoolFromIficmp(bc, BC_IFICMPG, OPERANDS_ON_STACK);
    } else if(node->kind() == tLE) {
        genBoolFromIficmp(bc, BC_IFICMPGE, OPERANDS_ON_STACK);
    }
    _typesStack.push_back(VT_INT);
    return true;
}

bool BytecodeGenerator::handleLogicOps(VarType leftOpType, BinaryOpNode* node, Bytecode* bc) {
    MaybeInsn mbLoad0 = typedInsnNumericsOnly(leftOpType, BC_ILOAD0, BC_DLOAD0);
    if(!mbLoad0.first) {
        return false;
    }
    Instruction bcLoad0 = mbLoad0.second;
    Instruction bcCmp = typedInsnNumericsOnly(leftOpType, BC_ICMP, BC_DCMP).second;
    genConvertToBool(bc, bcLoad0, bcCmp);
    bc->addInsn(BC_SWAP);
    genConvertToBool(bc, bcLoad0, bcCmp);
    bc->addInsn(BC_IADD);
    if(node->kind() == tOR) {
        bc->addInsn(BC_IAOR);
    } else if (node->kind() == tAND) {
        bc->addInsn(BC_IADD);
    } else {
        return false;
    }
    _typesStack.push_back(VT_INT);
    return true;
}

bool BytecodeGenerator::handleRangeOp(VarType leftOpType, BinaryOpNode* node, Bytecode* bc) {
    if(node->kind() != tRANGE) {
        return false;
    }
    _typesStack.push_back(leftOpType);
    return true;
}

void BytecodeGenerator::castIfNeeded(VarType valueType, StoreNode* node, Bytecode* bc, VarType varType) {
    if(varType == VT_INT && valueType == VT_DOUBLE) {
        bc->addInsn(BC_D2I);
    } else if (varType == VT_DOUBLE && valueType == VT_INT) {
        bc->addInsn(BC_I2D);
    }
}

void BytecodeGenerator::genAssignOpBc(StoreNode* node, Bytecode* bc, VarType varType) {
    Instruction bcOp = typedInsnNumericsOnly(varType, BC_IADD, BC_DADD).second;
    if(node->op() == tDECRSET) {
        bcOp = typedInsnNumericsOnly(varType, BC_ISUB, BC_DSUB).second;
    }
    bc->addInsn(bcOp);
}


void BytecodeGenerator::genBcInsnWithId(Bytecode* bc, Instruction insn, uint16_t id) {
    bc->addInsn(insn);
    bc->addInt16(id);
}

void BytecodeGenerator::genBcInsnWithTwoIds(Bytecode* bc, Instruction insn,
                                            uint16_t id1, uint16_t id2) {
    bc->addInsn(insn);
    bc->addInt16(id1);
    bc->addInt16(id2);
}

void BytecodeGenerator::genTransferDataBc(Bytecode* bc, Instruction transfInsn,
                                          vector<uint16_t> const& varsIds) {
    for(size_t i = 0; i != varsIds.size(); ++i) {
        genBcInsnWithId(bc, transfInsn, varsIds[i]);
    }
}

void BytecodeGenerator::genInsnNTimes(Bytecode* bc, Instruction insn, size_t n) {
    for(size_t i = 0; i != n; ++i) {
        bc->addInsn(insn);
    }
}

void BytecodeGenerator::genIncrementBc(Bytecode* bc, uint16_t varId,
                                       Instruction bcLoadVar, Instruction bcLoad1,
                                       Instruction bcAdd, Instruction bcStoreVar) {
    bc->addInsn(bcLoadVar);
    bc->addInt16(varId);
    bc->addInsn(bcLoad1);
    bc->addInsn(bcAdd);
    bc->addInsn(bcStoreVar);
    bc->addInt16(varId);
}

void BytecodeGenerator::genBoolFromIficmp(Bytecode* bc, Instruction ificmp, size_t popsNum) {
    Label labelTrue(bc);
    bc->addBranch(ificmp, labelTrue);
    genInsnNTimes(bc, BC_POP, popsNum);
    bc->addInsn(BC_ILOAD0);
    Label labelEnd(bc);
    bc->addBranch(BC_JA, labelEnd);
    bc->bind(labelTrue);
    genInsnNTimes(bc, BC_POP, popsNum);
    bc->addInsn(BC_ILOAD1);
    bc->bind(labelEnd);
}

void BytecodeGenerator::genConvertToBool(Bytecode* bc, Instruction bcLoad0, Instruction bcCmp) {
    bc->addInsn(bcLoad0);
    bc->addInsn(bcCmp);
    const size_t OPERANDS_ON_STACK = 2;
    genBoolFromIficmp(bc, BC_IFICMPNE, OPERANDS_ON_STACK);
}

BytecodeGenerator::MaybeInsn BytecodeGenerator::typedInsn(VarType varType,
                                                         Instruction intCaseInsn,
                                                         Instruction doubleCaseInsn,
                                                         Instruction stringCaseInsn) {
    if(varType == VT_INT) {
        return make_pair(true, intCaseInsn);
    } else if (varType == VT_DOUBLE) {
        return make_pair(true, doubleCaseInsn);
    } else if (varType == VT_STRING){
        return make_pair(true, stringCaseInsn);
    }
    return make_pair(false, BC_INVALID);
}

BytecodeGenerator::MaybeInsn BytecodeGenerator::typedInsnNumericsOnly(VarType varType,
                                                                     Instruction intCaseInsn,
                                                                     Instruction doubleCaseInsn) {
    if(varType == VT_INT) {
        return make_pair(true, intCaseInsn);
    } else if (varType == VT_DOUBLE) {
        return make_pair(true, doubleCaseInsn);
    }
    return make_pair(false, BC_INVALID);
}

}
