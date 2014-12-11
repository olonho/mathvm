#include "bytecode_generator.h"
#include "typechecking.h"

// == GeneratorState impl ==

uint16_t GeneratorState::currentCtxAddVar(const string &name, size_t nodePos) {
    Context& current = _contexts.back();
    uint16_t newId = (uint16_t)current.size();
    if(newId == UINT16_MAX) {
        throw ExceptionWithPos(tooMuchVarsMsg(), nodePos);
    }
    auto res = current.insert(make_pair(name, newId));
    if(!res.second) {
        throw ExceptionWithPos(varDoubleDeclMsg(name), nodePos);
    }
    return newId;
}

VarCoords GeneratorState::findVar(const string &name)  {
    VarCoords coords;
    coords.type = CT_NONE;
    for(size_t i = _contexts.size(); i > 0; --i) {
        Context& current = _contexts[i - 1];
        auto res = current.find(name);
        if(res != current.end()) {
            coords.varId = res->second;
            if(i == _contexts.size()) {
                coords.type = CT_LOCAL;
            } else {
                coords.type = CT_WITH_CTX;
                coords.ctxId = i - 1;
            }
            break;
        }
    }
    return coords;
}


// == BytecodeGenerator impl ==

void BytecodeGenerator::gen(AstFunction *top) {
    try {
        uint16_t id = _state.createBcFun(top);
        _state.pushFun(id);
        top->node()->visit(this);
        _state.popFun();
    } catch (ExceptionWithPos& e) {
        _status.setError(e.what(), e.source());
    } catch (ExceptionWithMsg& e) {
        _status.setError(e.what(), 0);
    }
}

void BytecodeGenerator::visitFunctionNode(FunctionNode* node) {
    if(node->body()->nodes() != 0 && node->body()->nodeAt(0)->isNativeCallNode()) {
        node->body()->nodeAt(0)->asNativeCallNode()->visit(this);
    } else {
        node->body()->visit(this);
    }
}

void BytecodeGenerator::visitBlockNode(BlockNode* node) {
    visitVarDecls(node);
    visitFunDefs(node);
    for(size_t i = 0; i != node->nodes(); ++i) {
        node->nodeAt(i)->visit(this);
    }
}

void BytecodeGenerator::visitReturnNode(ReturnNode* node) {
    Bytecode* bc = _state.currentBcToFill();
    if(node->returnExpr() != 0) {
        node->returnExpr()->visit(this);
        VarType expectedRetType = _state.currentBcFun()->returnType();
        VarType actualRetType = nodeType(node->returnExpr());
        if(actualRetType != expectedRetType) {
            Instruction castInsn = actualRetType == VT_INT ? BC_I2D : BC_D2I;
            bc->addInsn(castInsn);
        }
    }
    bc->addInsn(BC_RETURN);
}

void BytecodeGenerator::visitNativeCallNode(NativeCallNode *node) {
    // not implemented
}

void BytecodeGenerator::visitCallNode(CallNode* node) {
    BytecodeFunction* funToCall = _state.bcFunByName(node->name());
    assert(funToCall);
    Bytecode* bc = _state.currentBcToFill();
    for(size_t i = node->parametersNumber(); i > 0; --i) {
        node->parameterAt(i - 1)->visit(this);
        VarType expectedArgType = funToCall->parameterType(i - 1);
        VarType actualArgType = nodeType(node->parameterAt(i - 1));
        if(actualArgType != expectedArgType) {
            Instruction castInsn = actualArgType == VT_INT ? BC_I2D : BC_D2I;
            bc->addInsn(castInsn);
        }
    }
    bc->addInsn(BC_CALL);
    bc->addInt16(funToCall->id());
}

void BytecodeGenerator::visitPrintNode(PrintNode* node) {
    Bytecode* bc = _state.currentBcToFill();
    for(size_t i = 0; i != node->operands(); ++i) {
        node->operandAt(i)->visit(this);
        VarType argType = nodeType(node->operandAt(i));
        Instruction bcPrint = typedInsn(argType, BC_IPRINT, BC_DPRINT, BC_SPRINT);
        bc->addInsn(bcPrint);
    }
}

void BytecodeGenerator::visitUnaryOpNode(UnaryOpNode* node) {
    node->operand()->visit(this);
    Bytecode* bc = _state.currentBcToFill();
    TokenKind unOp = node->kind();
    VarType operandType = nodeType(node->operand());
    if(unOp == tSUB) {
        Instruction bcNeg = operandType == VT_INT ? BC_INEG : BC_DNEG;
        bc->addInsn(bcNeg);
    } else if (unOp == tNOT) {
        genNotBc(bc);
    } else if (unOp == tADD) {
        // unary '+' has no effect
    } else {
        throw ExceptionWithPos(invalidUnaryOperatorMsg(unOp), node->position());
    }
}

void BytecodeGenerator::visitBinaryOpNode(BinaryOpNode* node) {
    node->right()->visit(this);
    node->left()->visit(this);
    VarType rightOpType = nodeType(node->right());
    VarType leftOpType = nodeType(node->left());
    TokenKind binOp = node->kind();
    Bytecode* bc = _state.currentBcToFill();

    if(binOp == tADD) {
        genBaseArithmOpBc(bc, rightOpType, leftOpType, BC_IADD, BC_DADD);
    } else if(binOp == tSUB) {
        genBaseArithmOpBc(bc, rightOpType, leftOpType, BC_ISUB, BC_DSUB);
    } else if(binOp == tMUL) {
        genBaseArithmOpBc(bc, rightOpType, leftOpType, BC_IMUL, BC_DMUL);
    } else if(binOp == tDIV) {
        genBaseArithmOpBc(bc, rightOpType, leftOpType, BC_IDIV, BC_DDIV);
    } else if(binOp == tMOD) {
        bc->addInsn(BC_IMOD);
    } else if(binOp == tAAND) {
        bc->addInsn(BC_IAAND);
    } else if(binOp == tAOR) {
        bc->addInsn(BC_IAOR);
    } else if(binOp == tAXOR) {
        bc->addInsn(BC_IAXOR);
    } else if(binOp == tEQ) {
        getCompOpBc(bc, rightOpType, leftOpType, BC_IFICMPE, BC_IFICMPE);
    } else if(binOp == tNEQ) {
        getCompOpBc(bc, rightOpType, leftOpType, BC_IFICMPNE, BC_IFICMPNE);
    } else if(binOp == tGT) {
        getCompOpBc(bc, rightOpType, leftOpType, BC_IFICMPG, BC_IFICMPL);
    } else if(binOp == tGE) {
        getCompOpBc(bc, rightOpType, leftOpType, BC_IFICMPGE, BC_IFICMPLE);
    } else if(binOp == tLT) {
        getCompOpBc(bc, rightOpType, leftOpType, BC_IFICMPL, BC_IFICMPG);
    } else if(binOp == tLE) {
        getCompOpBc(bc, rightOpType, leftOpType, BC_IFICMPLE, BC_IFICMPGE);
    } else if(binOp == tOR){
        bc->addInsn(BC_IAOR);
        genConvertIntToBoolBc(bc);
    } else if(binOp == tAND) {
        bc->addInsn(BC_IAAND);
        genConvertIntToBoolBc(bc);
    } else if(binOp == tRANGE) {
        // no need to do anything
    } else {
        throw ExceptionWithPos(invalidBinOpMsg(node->kind()), node->position());
    }
}

void BytecodeGenerator::visitIntLiteralNode(IntLiteralNode* node) {
    Bytecode* bc = _state.currentBcToFill();
    bc->addInsn(BC_ILOAD);
    bc->addInt64(node->literal());
}

void BytecodeGenerator::visitDoubleLiteralNode(DoubleLiteralNode* node) {
    Bytecode* bc = _state.currentBcToFill();
    bc->addInsn(BC_DLOAD);
    bc->addDouble(node->literal());
}

void BytecodeGenerator::visitStringLiteralNode(StringLiteralNode* node) {
    uint16_t stringId = _state.makeStrConstant(node->literal());
    Bytecode* bc = _state.currentBcToFill();
    genBcInsnWithId(bc, BC_SLOAD, stringId);
}

void BytecodeGenerator::visitLoadNode(LoadNode* node) {
    string const& varName = node->var()->name();
    VarType const varType = node->var()->type();

    VarCoords coords = _state.findVar(varName);
    genLoadBc(varType, coords);
}

void BytecodeGenerator::visitStoreNode(StoreNode* node) {
    string const& varName = node->var()->name();
    VarType const varType = node->var()->type();
    node->value()->visit(this);
    VarType valueType = nodeType(node->value());

    Bytecode* bc = _state.currentBcToFill();
    if(valueType != varType) {
        Instruction castInsn = valueType == VT_INT ? BC_I2D : BC_D2I;
        bc->addInsn(castInsn);
    }
    VarCoords coords = _state.findVar(varName);
    TokenKind assignOp = node->op();
    if(assignOp != tASSIGN) {
        genLoadBc(varType, coords);
        Instruction bcOp;
        if(assignOp == tINCRSET) {
            bcOp = varType == VT_INT ? BC_IADD : BC_DADD;
        } else {
            bcOp = varType == VT_INT ? BC_ISUB : BC_DSUB;
        }
        bc->addInsn(bcOp);
    }
    genStoreBc(varType, coords);
}

void BytecodeGenerator::visitForNode(ForNode* node) {
    throwIfNotRangeOp(node);
    node->inExpr()->visit(this);
    Bytecode* bc = _state.currentBcToFill();
    // init iter var
    VarCoords iterVarCoords = _state.findVar(node->var()->name());
    genStoreBc(VT_INT, iterVarCoords);
    // begin for: check iter_var <= upper val (it is always on stack)
    Label forStart(bc);
    bc->bind(forStart);
    genLoadBc(VT_INT, iterVarCoords);
    bc->addInsn(BC_ICMP);
    bc->addInsn(BC_ILOAD1);
    Label forEnd(bc);
    bc->addBranch(BC_IFICMPE, forEnd);
    // then
    genRepeatInsn(bc, BC_POP, 3);
    node->body()->visit(this);
    // ++ iter_var
    genIntIncrBc(iterVarCoords);
    bc->addBranch(BC_JA, forStart);
    bc->bind(forEnd);
    // below we have 4 because of upper val
    genRepeatInsn(bc, BC_POP, 4);
}

void BytecodeGenerator::visitWhileNode(WhileNode* node) {
    Bytecode* bc = _state.currentBcToFill();
    Label whileStart(bc);
    bc->bind(whileStart);
    node->whileExpr()->visit(this);
    bc->addInsn(BC_ILOAD0);
    bc->addInsn(BC_ICMP);
    Label whileEnd(bc);
    bc->addBranch(BC_IFICMPE, whileEnd);
    genRepeatInsn(bc, BC_POP, 3);
    node->loopBlock()->visit(this);
    bc->addBranch(BC_JA, whileStart);
    bc->bind(whileEnd);
    genRepeatInsn(bc, BC_POP, 3);
}

void BytecodeGenerator::visitIfNode(IfNode* node) {
    Bytecode* bc = _state.currentBcToFill();
    node->ifExpr()->visit(this);
    bc->addInsn(BC_ILOAD0);
    bc->addInsn(BC_ICMP);
    Label labelElse(bc);
    bc->addBranch(BC_IFICMPE, labelElse);
    genRepeatInsn(bc, BC_POP, 3);
    node->thenBlock()->visit(this);
    Label labelAfterElse(bc);
    bc->addBranch(BC_JA, labelAfterElse);
    bc->bind(labelElse);
    genRepeatInsn(bc, BC_POP, 3);
    if(node->elseBlock() != 0) {
        node->elseBlock()->visit(this);
    }
    bc->bind(labelAfterElse);
}

// private:
// visitBlockNodeImpl
void BytecodeGenerator::visitVarDecls(BlockNode* node) {
    Bytecode* bc = _state.currentBcToFill();
    Scope::VarIterator varIt(node->scope());
    while(varIt.hasNext()) {
        AstVar* var = varIt.next();
        uint16_t varId = _state.currentCtxAddVar(var->name(), node->position());
        Instruction bcLoad0 = typedInsn(var->type(), BC_ILOAD0, BC_DLOAD0, BC_SLOAD0);
        bc->addInsn(bcLoad0);
        Instruction bcStore = typedInsn(var->type(), BC_STOREIVAR, BC_STOREDVAR, BC_STORESVAR);
        genBcInsnWithId(bc, bcStore, varId);
    }
}

void BytecodeGenerator::visitFunDefs(BlockNode* node) {
    deque<uint16_t> funIds;
    Scope::FunctionIterator it1(node->scope());
    while(it1.hasNext()) {
        uint16_t id = _state.createBcFun(it1.next());
        funIds.push_back(id);
    }
    Scope::FunctionIterator it2(node->scope());
    while(it2.hasNext()) {
        _state.pushFun(funIds.front());
        funIds.pop_front();
        FunctionNode* funNode = it2.next()->node();
        processFunArgs(funNode);
        funNode->visit(this);
        _state.popFun();
    }
}

void BytecodeGenerator::processFunArgs(FunctionNode* node) {
    Bytecode* bc = _state.currentBcToFill();
    for(size_t i = 0; i != node->parametersNumber(); ++i) {
        uint16_t id = _state.currentCtxAddVar(node->parameterName(i), node->position());
        Instruction bcStore = typedInsn(node->parameterType(i), BC_STOREIVAR, BC_STOREDVAR, BC_STORESVAR);
        genBcInsnWithId(bc, bcStore, id);
    }
}

// visitUnaryOpNode impl
void BytecodeGenerator::genNotBc(Bytecode* bc) {
    bc->addInsn(BC_ILOAD0);
    bc->addInsn(BC_ICMP);
    genBoolFromIficmp(bc, BC_IFICMPE, 3);
}

void BytecodeGenerator::genBoolFromIficmp(Bytecode* bc, Instruction ificmp, size_t popsNum) {
    Label labelTrue(bc);
    bc->addBranch(ificmp, labelTrue);
    genRepeatInsn(bc, BC_POP, popsNum);
    bc->addInsn(BC_ILOAD0);
    Label labelEnd(bc);
    bc->addBranch(BC_JA, labelEnd);
    bc->bind(labelTrue);
    genRepeatInsn(bc, BC_POP, popsNum);
    bc->addInsn(BC_ILOAD1);
    bc->bind(labelEnd);
}

void BytecodeGenerator::genRepeatInsn(Bytecode* bc, Instruction insn, size_t n) {
    for(size_t i = 0; i != n; ++i) {
        bc->addInsn(insn);
    }
}

// visitBinaryOpNode impl
void BytecodeGenerator::genBaseArithmOpBc(Bytecode* bc, VarType rightOpType, VarType leftOpType,
                                          Instruction intOp, Instruction doubleOp) {
    Instruction op = leftOpType == VT_INT ? intOp : doubleOp;
    if(rightOpType != leftOpType) {
        genOperandsCastBc(bc, leftOpType);
        op = doubleOp;
    }
    bc->addInsn(op);

}

void BytecodeGenerator::genOperandsCastBc(Bytecode* bc, VarType leftOpType) {
    if(leftOpType == VT_INT) {
        bc->addInsn(BC_I2D);
    } else {
        bc->addInsn(BC_SWAP);
        bc->addInsn(BC_I2D);
        bc->addInsn(BC_SWAP);
    }
}

void BytecodeGenerator::getCompOpBc(Bytecode* bc, VarType rightOpType, VarType leftOpType,
                                    Instruction intIfcmp, Instruction doubleIfcmp) {
    if(rightOpType != leftOpType) {
        genOperandsCastBc(bc, leftOpType);
        leftOpType = rightOpType = VT_DOUBLE;
    }
    Instruction ifcmp = intIfcmp;
    size_t popsNum = 2;
    if(leftOpType == VT_DOUBLE) {
        bc->addInsn(BC_DCMP);
        bc->addInsn(BC_DLOAD0);
        ifcmp = doubleIfcmp;
        popsNum += 2;
    }
    genBoolFromIficmp(bc, ifcmp, popsNum);
}

void BytecodeGenerator::genConvertIntToBoolBc(Bytecode* bc) {
    bc->addInsn(BC_ILOAD0);
    bc->addInsn(BC_ICMP);
    genBoolFromIficmp(bc, BC_IFICMPNE, 3);
}

// visitStringLiteralNode impl
void BytecodeGenerator::genBcInsnWithId(Bytecode* bc, Instruction insn, uint16_t id) {
    bc->addInsn(insn);
    bc->addInt16(id);
}

// visitLoadNode impl
void BytecodeGenerator::genTransferBc(VarType type, VarCoords& coords,
                                      Instruction transIVar, Instruction transDVar, Instruction transSVar,
                                      Instruction transCtxIVar, Instruction transCtxDVar, Instruction transCtxSVar) {
    Bytecode* bc = _state.currentBcToFill();
    assert(coords.type != CT_NONE);
    if(coords.type == CT_LOCAL) {
        Instruction bcTransvar = typedInsn(type, transIVar, transDVar, transSVar);
        genBcInsnWithId(bc, bcTransvar, coords.varId);
    } else {
        Instruction bcTransctxvar = typedInsn(type, transCtxIVar, transCtxDVar, transCtxSVar);
        genBcInsnWithTwoIds(bc, bcTransctxvar, coords.ctxId, coords.varId);
    }
}

void BytecodeGenerator::genBcInsnWithTwoIds(Bytecode* bc, Instruction insn,
                                            uint16_t id1, uint16_t id2) {
    bc->addInsn(insn);
    bc->addInt16(id1);
    bc->addInt16(id2);
}

// visitForNode impl
void BytecodeGenerator::throwIfNotRangeOp(ForNode *node) {
    AstNode* inExp = node->inExpr();
    uint32_t nodePos = node->position();
    if(!inExp->isBinaryOpNode()) {
        throw ExceptionWithPos(wrongInExprMsg(), nodePos);
    }
    TokenKind inExprOpKind = inExp->asBinaryOpNode()->kind();
    if(inExprOpKind != tRANGE) {
        throw ExceptionWithPos(wrongInExprMsg(), nodePos);
    }
}

void BytecodeGenerator::genIntIncrBc(VarCoords& intVarCoords) {
    Bytecode* bc = _state.currentBcToFill();
    genLoadBc(VT_INT, intVarCoords);
    bc->addInsn(BC_ILOAD1);
    bc->addInsn(BC_IADD);
    genStoreBc(VT_INT, intVarCoords);
}
