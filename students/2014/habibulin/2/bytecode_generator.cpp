#include "bytecode_generator.h"
#include "interpreter_code_impl.h"
#include "my_utils.h"

#include <memory>
#include <deque>

using std::shared_ptr;
using std::deque;

namespace mathvm {

void BytecodeGenerator::visitProgram(AstFunction* astFun) {
    DEBUG_MSG("visitProgram start");
    DEBUG_MSG(_typesStack);

    BytecodeFunction* bcFun = initFun(astFun);
    _funIdsStack.push_back(bcFun->id());
    genFunBc(bcFun->bytecode(), astFun->node());
    _funIdsStack.pop_back();

    DEBUG_MSG("visitProgram end");
    DEBUG_MSG(_typesStack);
}

void BytecodeGenerator::visitFunctionNode(FunctionNode* node) {
    DEBUG_MSG("visitFunctionNode start: " + currentBcFunction()->name());
    DEBUG_MSG(_typesStack);
    if(node->body()->nodes() != 0 && node->body()->nodeAt(0)->isNativeCallNode()) {
        node->body()->nodeAt(0)->asNativeCallNode()->visit(this);
    } else {
        node->body()->visit(this);
        _typesStack.pop_back();
    }
    DEBUG_MSG("visitFunctionNode end: " + currentBcFunction()->name());
    DEBUG_MSG(_typesStack);
}

void BytecodeGenerator::visitNativeCallNode(NativeCallNode *node) {
    DEBUG_MSG("visitNativeCallNode start");
    DEBUG_MSG(_typesStack);
    // not implemented
    // push return type
    DEBUG_MSG("visitNativeCallNode end");
    DEBUG_MSG(_typesStack);
}

void BytecodeGenerator::visitBlockNode(BlockNode* node) {
    DEBUG_MSG("visitBlockNode start");
    DEBUG_MSG(_typesStack);
    visitVarDecls(node);
    currentBcFunction()->setLocalsNumber(currentCtx()->varsNumber() - currentBcFunction()->parametersNumber());
    visitFunDefs(node);
    visitExprs(node);
    _typesStack.push_back(VT_VOID);
    DEBUG_MSG("visitBlockNode end");
    DEBUG_MSG(_typesStack);
}

void BytecodeGenerator::visitUnaryOpNode(UnaryOpNode* node) {
    DEBUG_MSG("visitUnaryOpNode start");
    DEBUG_MSG(_typesStack);
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
    DEBUG_MSG("visitUnaryOpNode end");
    DEBUG_MSG(_typesStack);
}

void BytecodeGenerator::visitBinaryOpNode(BinaryOpNode* node) {
    DEBUG_MSG("visitBinaryOpNode start");
    DEBUG_MSG(_typesStack);
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
    DEBUG_MSG("visitBinaryOpNode end");
    DEBUG_MSG(_typesStack);
}

void BytecodeGenerator::visitLoadNode(LoadNode* node) {
    DEBUG_MSG("visitLoadNode start: " + node->var()->name());
    DEBUG_MSG(_typesStack);
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
    DEBUG_MSG("visitLoadNode end: " + node->var()->name());
    DEBUG_MSG(_typesStack);
}

void BytecodeGenerator::visitStoreNode(StoreNode* node) {
    DEBUG_MSG("visitStoreNode start: " + node->var()->name());
    DEBUG_MSG(_typesStack);
    string const& varName = node->var()->name();
    VarType varType = node->var()->type();

    node->value()->visit(this);
    VarType valueType = _typesStack.back();
    _typesStack.pop_back();

    Bytecode* bc = currentBcToFill();
    throwIfTypesIncompatible(node->position(), varType, valueType);
    castIfNeeded(valueType, node, bc, varType);

    shared_ptr<Context> curCtx = currentCtx();
    genStoreNodeBc(bc, varName, varType, curCtx, node);
    _typesStack.push_back(varType);
    DEBUG_MSG("visitStoreNode end: " + node->var()->name());
    DEBUG_MSG(_typesStack);
}

void BytecodeGenerator::visitIntLiteralNode(IntLiteralNode* node) {
    DEBUG_MSG("visitIntLiteralNode start");
    DEBUG_MSG(_typesStack);
    Bytecode* bc = currentBcToFill();
    DEBUG_MSG(bc);
    bc->addInsn(BC_ILOAD);
    bc->addInt64(node->literal());
    _typesStack.push_back(VT_INT);
    DEBUG_MSG("visitIntLiteralNode end");
    DEBUG_MSG(_typesStack);
}

void BytecodeGenerator::visitDoubleLiteralNode(DoubleLiteralNode* node) {
    DEBUG_MSG("visitIntLiteralNode start");
    DEBUG_MSG(_typesStack);
    Bytecode* bc = currentBcToFill();
    bc->addInsn(BC_DLOAD);
    bc->addDouble(node->literal());
    _typesStack.push_back(VT_DOUBLE);
    DEBUG_MSG("visitIntLiteralNode end");
    DEBUG_MSG(_typesStack);
}

void BytecodeGenerator::visitStringLiteralNode(StringLiteralNode* node) {
    DEBUG_MSG("visitIntLiteralNode start");
    DEBUG_MSG(_typesStack);
    uint16_t stringId = _code->makeStringConstant(node->literal());
    Bytecode* bc = currentBcToFill();
    genBcInsnWithId(bc, BC_SLOAD, stringId);
    _typesStack.push_back(VT_STRING);
    DEBUG_MSG("visitIntLiteralNode end");
    DEBUG_MSG(_typesStack);
}

void BytecodeGenerator::visitPrintNode(PrintNode* node) {
    DEBUG_MSG("visitPrintNode start");
    DEBUG_MSG(_typesStack);
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
    DEBUG_MSG("visitPrintNode end");
    DEBUG_MSG(_typesStack);
}

void BytecodeGenerator::visitCallNode(CallNode* node) {
    DEBUG_MSG("visitCallNode start: " + node->name());
    DEBUG_MSG(_typesStack);

    TranslatedFunction* funToCall = _code->functionByName(node->name());
    if(!funToCall) {
        throw TranslatorException(undefFunMsg(node->name()), node->position());
    }
    visitCallArguments(node, funToCall);
    Bytecode* bc = currentBcToFill();
    bc->addInsn(BC_CALL);
    bc->addInt16(funToCall->id());
    for(size_t i = node->parametersNumber(); i > 0; --i) {
        _typesStack.pop_back();
    }
    _typesStack.push_back(funToCall->returnType());

    DEBUG_MSG("visitCallNode end: " + node->name());
    DEBUG_MSG(_typesStack);
}

void BytecodeGenerator::visitReturnNode(ReturnNode* node) {
    DEBUG_MSG("visitReturnNode start");
    DEBUG_MSG(_typesStack);

    Bytecode* bc = currentBcToFill();
    if(node->returnExpr() == 0) {
        _typesStack.push_back(VT_VOID);
    } else {
        node->returnExpr()->visit(this);
        VarType retType = currentBcFunction()->returnType();
        if(retType != _typesStack.back()) {
            string const throwMsg = invalidTypeToReturn(currentBcFunction()->name(), retType, _typesStack.back());
            genCastBcOrThrow(bc, retType, _typesStack.back(), throwMsg);
        }
    }
    bc->addInsn(BC_RETURN);

    DEBUG_MSG("visitReturnNode end");
    DEBUG_MSG(_typesStack);
}

void BytecodeGenerator::visitIfNode(IfNode* node) {
    DEBUG_MSG("visitIfNode start");
    DEBUG_MSG(_typesStack);
    Bytecode* bc = currentBcToFill();
    node->ifExpr()->visit(this);
    if(_typesStack.back() != VT_INT) {
        throw TranslatorException(wrongTypeMsg(VT_INT), node->ifExpr()->position());
    }
    _typesStack.pop_back();
    bc->addInsn(BC_ILOAD0);
    bc->addInsn(BC_ICMP);
    Label labelElse(bc);
    bc->addBranch(BC_IFICMPE, labelElse);
    const size_t COND_CHECK_RESULTS_ON_STACK = 3;
    genInsnNTimes(bc, BC_POP, COND_CHECK_RESULTS_ON_STACK);
    node->thenBlock()->visit(this);
    _typesStack.pop_back();
    Label labelAfterElse(bc);
    bc->addBranch(BC_JA, labelAfterElse);
    bc->bind(labelElse);
    genInsnNTimes(bc, BC_POP, COND_CHECK_RESULTS_ON_STACK);
    if(node->elseBlock() != 0) {
        node->elseBlock()->visit(this);
        _typesStack.pop_back();
    }
    bc->bind(labelAfterElse);
    _typesStack.push_back(VT_VOID);
    DEBUG_MSG("visitIfNode end");
    DEBUG_MSG(_typesStack);
}

void BytecodeGenerator::visitWhileNode(WhileNode* node) {
    DEBUG_MSG("visitWhileNode start");
    DEBUG_MSG(_typesStack);
    Bytecode* bc = currentBcToFill();
    Label whileStart(bc);
    bc->bind(whileStart);
    node->whileExpr()->visit(this);
    if(_typesStack.back() != VT_INT) {
        throw TranslatorException(wrongTypeMsg(VT_INT), node->whileExpr()->position());
    }
    _typesStack.pop_back();
    bc->addInsn(BC_ILOAD0);
    bc->addInsn(BC_ICMP);
    Label whileEnd(bc);
    bc->addBranch(BC_IFICMPE, whileEnd);
    const size_t COND_CHECK_RESULTS_ON_STACK = 3;
    genInsnNTimes(bc, BC_POP, COND_CHECK_RESULTS_ON_STACK);
    node->loopBlock()->visit(this);
    _typesStack.pop_back();
    bc->addBranch(BC_JA, whileStart);
    bc->bind(whileEnd);
    genInsnNTimes(bc, BC_POP, COND_CHECK_RESULTS_ON_STACK);
    _typesStack.push_back(VT_VOID);
    DEBUG_MSG("visitWhileNode end");
    DEBUG_MSG(_typesStack);
}

void BytecodeGenerator::visitForNode(ForNode* node) {
    DEBUG_MSG("visitForNode start: " + node->var()->name());
    DEBUG_MSG(_typesStack);

    string const& iterVarName = node->var()->name();
    if(node->var()->type() != VT_INT) {
        throw TranslatorException(wrongIterVarTypeMsg(iterVarName), node->position());
    }
    shared_ptr<Context> ctx = currentCtx();
    if(!ctx->hasVar(iterVarName)) {
        throw TranslatorException(varNotDeclaredMsg(iterVarName), node->position());
    }
    uint16_t iterVarId = ctx->getVarId(iterVarName);

    if(!node->inExpr()->isBinaryOpNode()) {
        throw TranslatorException(wrongExprInForNode(), node->position());
    }
    TokenKind inExprOpKind = node->inExpr()->asBinaryOpNode()->kind();
    if(inExprOpKind != tRANGE) {
        throw TranslatorException(invalidOpInForNode(inExprOpKind), node->inExpr()->position());
    }
    node->inExpr()->visit(this);
    _typesStack.pop_back();
    genForNodeBc(currentBcToFill(), node, iterVarId);
    _typesStack.push_back(VT_INVALID);

    DEBUG_MSG("visitForNode end: " + node->var()->name());
    DEBUG_MSG(_typesStack);
}

// private methods section

void BytecodeGenerator::visitVarDecls(BlockNode* node) {
    DEBUG_MSG("visitVarDecls start");
    DEBUG_MSG(_typesStack);
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
    DEBUG_MSG("visitVarDecls end");
    DEBUG_MSG(_typesStack);
}

void BytecodeGenerator::visitFunDefs(BlockNode* node) {
    DEBUG_MSG("visitFunDefs start");
    DEBUG_MSG(_typesStack);

    deque<uint16_t> newFunsIds;
    Scope::FunctionIterator initFunIt(node->scope());
    while(initFunIt.hasNext()) {
        BytecodeFunction* bcFun = initFun(initFunIt.next());
        newFunsIds.push_back(bcFun->id());
    }
    Scope::FunctionIterator genFunBcIt(node->scope());
    while(genFunBcIt.hasNext()) {
        _funIdsStack.push_back(newFunsIds.front());
        genFunBc(currentBcToFill(), genFunBcIt.next()->node());
        _funIdsStack.pop_back();
        newFunsIds.pop_front();
    }

    DEBUG_MSG("visitFunDefs end");
    DEBUG_MSG(_typesStack);
}

void BytecodeGenerator::visitExprs(BlockNode* node) {
    DEBUG_MSG("visitExprs start");
    DEBUG_MSG(_typesStack);
    for(size_t i = 0; i != node->nodes(); ++i) {
        node->nodeAt(i)->visit(this);
        _typesStack.pop_back();
    }
    DEBUG_MSG("visitExprs end");
    DEBUG_MSG(_typesStack);
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
    }
    return newCtx;
}

shared_ptr<Context> BytecodeGenerator::findVarInOuterCtx(string const& name) {
    if(_funIdsStack.size() > 1) {
        for(auto it = _funIdsStack.rbegin() + 1; it != _funIdsStack.rend(); ++it) {
            shared_ptr<Context> outerCtx = _code->ctxById(*it);
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

void BytecodeGenerator::genForNodeBc(Bytecode* bc, ForNode* node, uint16_t iterVarId) {
    // init iter var
    genBcInsnWithId(bc, BC_STOREIVAR, iterVarId);
    // begin for: check iter_var <= upper val (it is always on stack)
    Label forStart(bc);
    bc->bind(forStart);
    genBcInsnWithId(bc, BC_LOADIVAR, iterVarId);
    bc->addInsn(BC_ICMP);
    bc->addInsn(BC_ILOAD1);
    Label forEnd(bc);
    bc->addBranch(BC_IFICMPE, forEnd);
    // then
    const size_t RESULTS_ON_STACK = 3;
    genInsnNTimes(bc, BC_POP, RESULTS_ON_STACK);
    node->body()->visit(this);
    // ++ iter_var
    genIncrementBc(bc, iterVarId, BC_LOADIVAR, BC_ILOAD1, BC_IADD, BC_STOREIVAR);
    bc->addBranch(BC_JA, forStart);
    bc->bind(forEnd);
    // below we have +1 for upper val
    genInsnNTimes(bc, BC_POP, RESULTS_ON_STACK + 1);
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
        bc->addInsn(BC_I2D);
        leftOpType = VT_DOUBLE;
    } else if(leftOpType == VT_DOUBLE && rightOpType == VT_INT) {
        bc->addInsn(BC_SWAP);
        bc->addInsn(BC_I2D);
        bc->addInsn(BC_SWAP);
        rightOpType = VT_DOUBLE;
    }
}

bool BytecodeGenerator::handleBaseArithmOps(VarType leftOpType, BinaryOpNode* node, Bytecode* bc) {
    DEBUG_MSG("handleBaseArithmOps");
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
    DEBUG_MSG("handleIntArithmOps");
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
    DEBUG_MSG("handleIntCompOps");
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
    } else {
        return false;
    }
    _typesStack.push_back(VT_INT);
    return true;
}

bool BytecodeGenerator::handleDoubleCompOps(VarType leftOpType, BinaryOpNode* node, Bytecode* bc) {
    DEBUG_MSG("handleDoubleCompOps");
    if(leftOpType != VT_DOUBLE) {
        return false;
    }
    const size_t OPERANDS_ON_STACK = 4;
    if(node->kind() == tEQ) {
        bc->addInsn(BC_DCMP);
        bc->addInsn(BC_DLOAD0);
        genBoolFromIficmp(bc, BC_IFICMPE, OPERANDS_ON_STACK);
    } else if(node->kind() == tNEQ) {
        bc->addInsn(BC_DCMP);
        bc->addInsn(BC_DLOAD0);
        genBoolFromIficmp(bc, BC_IFICMPNE, OPERANDS_ON_STACK);
    } else if(node->kind() == tGT) {
        bc->addInsn(BC_DCMP);
        bc->addInsn(BC_DLOAD0);
        genBoolFromIficmp(bc, BC_IFICMPL, OPERANDS_ON_STACK);
    } else if(node->kind() == tGE) {
        bc->addInsn(BC_DCMP);
        bc->addInsn(BC_DLOAD0);
        genBoolFromIficmp(bc, BC_IFICMPLE, OPERANDS_ON_STACK);
    } else if(node->kind() == tLT) {
        bc->addInsn(BC_DCMP);
        bc->addInsn(BC_DLOAD0);
        genBoolFromIficmp(bc, BC_IFICMPG, OPERANDS_ON_STACK);
    } else if(node->kind() == tLE) {
        bc->addInsn(BC_DCMP);
        bc->addInsn(BC_DLOAD0);
        genBoolFromIficmp(bc, BC_IFICMPGE, OPERANDS_ON_STACK);
    } else {
        return false;
    }
    _typesStack.push_back(VT_INT);
    return true;
}

bool BytecodeGenerator::handleLogicOps(VarType leftOpType, BinaryOpNode* node, Bytecode* bc) {
    DEBUG_MSG("handleLogicOps");
    MaybeInsn mbLoad0 = typedInsnNumericsOnly(leftOpType, BC_ILOAD0, BC_DLOAD0);
    if(!mbLoad0.first) {
        return false;
    }
    if(node->kind() != tOR && node->kind() != tAND) {
        return false;
    }
    Instruction bcLoad0 = mbLoad0.second;
    Instruction bcCmp = typedInsnNumericsOnly(leftOpType, BC_ICMP, BC_DCMP).second;
    genConvertToBool(bc, bcLoad0, bcCmp);
    bc->addInsn(BC_SWAP);
    genConvertToBool(bc, bcLoad0, bcCmp);
    if(node->kind() == tOR) {
        bc->addInsn(BC_IAOR);
    } else if (node->kind() == tAND) {
        bc->addInsn(BC_IAAND);
    } else {
        return false;
    }
    _typesStack.push_back(VT_INT);
    return true;
}

bool BytecodeGenerator::handleRangeOp(VarType leftOpType, BinaryOpNode* node, Bytecode* bc) {
    DEBUG_MSG("handleRangeOp");
    if(node->kind() != tRANGE) {
        return false;
    }
    _typesStack.push_back(VT_INVALID);
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

void BytecodeGenerator::genStoreNodeBc(Bytecode* bc, string const& varName, VarType varType,
                                       shared_ptr<Context> curCtx, StoreNode* node) {
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
    const size_t OPERANDS_ON_STACK = 3;
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

// visitProgramImpl

BytecodeFunction* BytecodeGenerator::initFun(AstFunction* astFun) {
    DEBUG_MSG("initFun start");
    DEBUG_MSG(_typesStack);

    BytecodeFunction* bcFun = createBytecodeFun(astFun);
    createContextWithArgs(astFun->node(), bcFun->id());

    DEBUG_MSG("initFun end");
    DEBUG_MSG(_typesStack);

    return bcFun;
}

void BytecodeGenerator::genFunBc(Bytecode *bc, FunctionNode *funNode) {
    genArgsStoreBc(bc, funNode);
    funNode->visit(this);
}

// visitCallNode impl

void BytecodeGenerator::visitCallArguments(CallNode* node, TranslatedFunction* funToCall) {
    for(size_t i = node->parametersNumber(); i > 0; --i) {
        node->parameterAt(i - 1)->visit(this);
        VarType paramType = funToCall->parameterType(i - 1);
        string const throwMsg = invalidFunArgType(funToCall->name(), paramType, _typesStack.back());
        if(paramType != _typesStack.back()) {
            genCastBcOrThrow(currentBcToFill(), paramType, _typesStack.back(), throwMsg);
        }
    }
}

void BytecodeGenerator::genCastBcOrThrow(Bytecode* bc, VarType expectedType, VarType actualType, string const& throwMsg) {
    MaybeInsn mbCastInsn = getFstToSndCastBc(actualType, expectedType);
    if(!mbCastInsn.first) {
        throw TranslatorException(throwMsg, 0);
    }
    bc->addInsn(mbCastInsn.second);
}

BytecodeGenerator::MaybeInsn BytecodeGenerator::getFstToSndCastBc(VarType fstType, VarType sndType) {
    if((fstType == VT_INT || fstType == VT_DOUBLE) && sndType == VT_DOUBLE) {
        return MaybeInsn(true, BC_I2D);
    } else if ((fstType == VT_INT || fstType == VT_DOUBLE) && sndType == VT_INT) {
        return MaybeInsn(true, BC_D2I);
    }
    return MaybeInsn(false, BC_INVALID);
}

// visitForNode impl



}
