#include "bc_gen.h"

using namespace mathvm;

Bytecode* BytecodeGenerator::bc() {
    return _functions.top()->bytecode();
}

static void error(AstNode* node = nullptr, const string& message = "") {
    (new CodeGenError())->error(node ? node->position() : 0, "CodeGenError: %s", message.c_str());
}

static VarType combineTypes(VarType arg1, VarType arg2) {
    return (arg1 == VT_INT && arg2 == VT_INT) ? VT_INT : VT_DOUBLE;
}

void caseIntDouble(VarType type, Instruction iInsn, Instruction dInsn, Bytecode* bc) {
    switch (type) {
        case VT_INT:
            bc->addInsn(iInsn);
            break;
        case VT_DOUBLE:
            bc->addInsn(dInsn);
            break;
    }
}

void caseIntDoubleString(VarType type, Instruction iInsn, Instruction dInsn, Instruction sInsn, Bytecode* bc) {
    switch (type) {
        case VT_INT:
            bc->addInsn(iInsn);
            break;
        case VT_DOUBLE:
            bc->addInsn(dInsn);
            break;
        case VT_STRING:
            bc->addInsn(sInsn);
            break;
    }
}

void cast(VarType to, VarType from, Bytecode* bc) {
    if (from == to) {
        return;
    } else if (from == VT_INT && to == VT_DOUBLE) {
        bc->addInsn(BC_I2D);
    } else if (from == VT_DOUBLE && to == VT_INT) {
        bc->addInsn(BC_D2I);
    } /*else if (from == VT_STRING && to == VT_INT) {
        bc->addInsn(BC_S2I);
    }*/
}

void castIntToBool(Bytecode* bc) {
    Label endLabel(bc);

    bc->addInsn(BC_ILOAD0);
    bc->addBranch(BC_IFICMPE, endLabel);
    bc->addInsn(BC_ILOAD1);
    bc->addInsn(BC_SWAP);
    bc->addInsn(BC_POP);
    bc->addInsn(BC_SWAP);

    bc->bind(endLabel);
    bc->addInsn(BC_POP);
}

void compare(VarType type, TokenKind kind, Bytecode* bc) {
    bc->addInsn(BC_SWAP);

    if (type == VT_DOUBLE) {
        bc->addInsn(BC_DCMP);

        bc->addInsn(BC_SWAP);
        bc->addInsn(BC_POP);
        bc->addInsn(BC_SWAP);
        bc->addInsn(BC_POP);

        bc->addInsn(BC_ILOAD0);
        bc->addInsn(BC_SWAP);
    }

    Label trueValue(bc);
    Label endLabel(bc);

    switch (kind) {
        case tEQ:
            bc->addBranch(BC_IFICMPE, trueValue);
            break;
        case tNEQ:
            bc->addBranch(BC_IFICMPNE, trueValue);
            break;
        case tGT:
            bc->addBranch(BC_IFICMPG, trueValue);
            break;
        case tGE:
            bc->addBranch(BC_IFICMPGE, trueValue);
            break;
        case tLT:
            bc->addBranch(BC_IFICMPL, trueValue);
            break;
        case tLE:
            bc->addBranch(BC_IFICMPLE, trueValue);
            break;
    }

    bc->addInsn(BC_POP);
    bc->addInsn(BC_POP);
    bc->addInsn(BC_ILOAD0);
    bc->addBranch(BC_JA, endLabel);

    bc->bind(trueValue);
    bc->addInsn(BC_POP);
    bc->addInsn(BC_POP);
    bc->addInsn(BC_ILOAD1);

    bc->bind(endLabel);
}

void BytecodeGenerator::visitBinaryOpNode(BinaryOpNode* node) {
    VarType leftType = _preprocessor->getType(node->left());
    VarType rightType = _preprocessor->getType(node->right());
    VarType argType = combineTypes(leftType, rightType);
    TokenKind kind = node->kind();

    if (kind == tOR || kind == tAND) {
        node->left()->visit(this);
        castIntToBool(bc());

        Label endLabel(bc());

        bc()->addInsn(BC_ILOAD0);
        switch (kind) {
            case tOR:
                bc()->addBranch(BC_IFICMPNE, endLabel);
                break;
            case tAND:
                bc()->addBranch(BC_IFICMPE, endLabel);
                break;
        }

        bc()->addInsn(BC_POP);
        node->right()->visit(this);
        castIntToBool(bc());
        bc()->addInsn(BC_SWAP);

        bc()->bind(endLabel);
        bc()->addInsn(BC_POP);  // нолик с которым сравнивали
        return;
    }

    node->left()->visit(this);
    cast(argType, leftType, bc());
    node->right()->visit(this);
    cast(argType, rightType, bc());

    switch (kind) {
        case tEQ:
        case tNEQ:
        case tGT:
        case tGE:
        case tLT:
        case tLE:
            compare(argType, kind, bc());
            break;

        case tADD:
            caseIntDouble(argType, BC_IADD, BC_DADD, bc());
            break;
        case tSUB:
            bc()->addInsn(BC_SWAP);
            caseIntDouble(argType, BC_ISUB, BC_DSUB, bc());
            break;
        case tMUL:
            caseIntDouble(argType, BC_IMUL, BC_DMUL, bc());
            break;
        case tDIV:
            bc()->addInsn(BC_SWAP);
            caseIntDouble(argType, BC_IDIV, BC_DDIV, bc());
            break;

        case tMOD:
            bc()->addInsn(BC_SWAP);
            bc()->addInsn(BC_IMOD);
            break;

        case tAOR:
            bc()->addInsn(BC_IAOR);
            break;
        case tAAND:
            bc()->addInsn(BC_IAAND);
            break;
        case tAXOR:
            bc()->addInsn(BC_IAXOR);
            break;

        case tRANGE:
            break;
    }
}

void BytecodeGenerator::visitUnaryOpNode(UnaryOpNode* node) {
    node->operand()->visit(this);
    VarType type = _preprocessor->getType(node->operand());

    switch (node->kind()) {
        case tSUB:
            caseIntDouble(type, BC_INEG, BC_DNEG, bc());
            break;

        case tNOT:
            if (type == VT_STRING) {
                bc()->addInsn(BC_S2I);
            }
            castIntToBool(bc());
            bc()->addInsn(BC_ILOAD1);
            bc()->addInsn(BC_IAXOR);
            break;
    }
}

void BytecodeGenerator::visitStringLiteralNode(StringLiteralNode* node) {
    uint16_t id = _code->makeStringConstant(node->literal());

    bc()->addInsn(BC_SLOAD);
    bc()->addUInt16(id);
}

void BytecodeGenerator::visitDoubleLiteralNode(DoubleLiteralNode* node) {
    bc()->addInsn(BC_DLOAD);
    bc()->addDouble(node->literal());
}

void BytecodeGenerator::visitIntLiteralNode(IntLiteralNode* node) {
    bc()->addInsn(BC_ILOAD);
    bc()->addInt64(node->literal());
}

uint16_t BytecodeGenerator::getScopeId(const AstVar* var) {
    if (!_var2scope.count(var)) error(nullptr, "scopeId");
    return _var2scope[var];
}

uint16_t BytecodeGenerator::getVarId(const AstVar* var) {
    if (!_var2id.count(var)) error(nullptr, "varId");
    return _var2id[var];
}

void BytecodeGenerator::loadVar(const AstVar* var) {
    uint16_t scopeId = getScopeId(var);
    uint16_t varId = getVarId(var);
    VarType varType = var->type();

    /*if (scopeId == _functions.top()->scopeId()) {
        caseIntDoubleString(var->type(), BC_LOADIVAR, BC_LOADDVAR, BC_LOADSVAR, bc());
    } else {
        caseIntDoubleString(var->type(), BC_LOADCTXIVAR, BC_LOADCTXDVAR, BC_LOADCTXSVAR, bc());
        bc()->addUInt16(scopeId);
    }*/
    caseIntDoubleString(varType, BC_LOADCTXIVAR, BC_LOADCTXDVAR, BC_LOADCTXSVAR, bc());
    bc()->addUInt16(scopeId);
    bc()->addUInt16(varId);
}

void BytecodeGenerator::storeVar(const AstVar* var) {
    uint16_t scopeId = getScopeId(var);
    uint16_t varId = getVarId(var);
    VarType varType = var->type();

    caseIntDoubleString(varType, BC_STORECTXIVAR, BC_STORECTXDVAR, BC_STORECTXSVAR, bc());
    bc()->addUInt16(scopeId);
    bc()->addUInt16(varId);
}

void BytecodeGenerator::visitLoadNode(LoadNode* node) {
    loadVar(node->var());
}

void BytecodeGenerator::visitStoreNode(StoreNode* node) {
    node->value()->visit(this);

    const AstVar* var = node->var();
    VarType varType = var->type();
    VarType valueType = _preprocessor->getType(node->value());
    cast(varType, valueType, bc());

    switch (node->op()) {
        case tEQ:
            break;

        case tINCRSET:
            loadVar(var);
            caseIntDouble(varType, BC_IADD, BC_DADD, bc());
            break;

        case tDECRSET:
            loadVar(var);
            caseIntDouble(varType, BC_ISUB, BC_DSUB, bc());
            break;
    }

    storeVar(var);
}

void BytecodeGenerator::visitForNode(ForNode* node) {
    Label condition(bc());
    Label end(bc());

    node->inExpr()->visit(this);
    bc()->addInsn(BC_SWAP);
    storeVar(node->var());
    loadVar(node->var());

    bc()->bind(condition);
    bc()->addBranch(BC_IFICMPG, end);

    node->body()->visit(this);
    bc()->addInsn(BC_ILOAD1);
    bc()->addInsn(BC_IADD);
    storeVar(node->var());
    loadVar(node->var());
    bc()->addBranch(BC_JA, condition);

    bc()->bind(end);
    bc()->addInsn(BC_POP);
    bc()->addInsn(BC_POP);
}

void BytecodeGenerator::visitWhileNode(WhileNode* node) {
    Label condition(bc());
    Label end(bc());

    bc()->bind(condition);
    node->whileExpr()->visit(this);
    bc()->addInsn(BC_ILOAD0);
    bc()->addBranch(BC_IFICMPE, end);

    bc()->addInsn(BC_POP);
    bc()->addInsn(BC_POP);
    node->loopBlock()->visit(this);
    bc()->addBranch(BC_JA, condition);

    bc()->bind(end);
    bc()->addInsn(BC_POP);
    bc()->addInsn(BC_POP);
}

void BytecodeGenerator::visitIfNode(IfNode* node) {
    Label elseCase(bc());
    Label end(bc());

    node->ifExpr()->visit(this);
    bc()->addInsn(BC_ILOAD0);
    bc()->addBranch(BC_IFICMPE, elseCase);

    bc()->addInsn(BC_POP);
    bc()->addInsn(BC_POP);
    node->thenBlock()->visit(this);
    bc()->addBranch(BC_JA, end);

    bc()->bind(elseCase);
    bc()->addInsn(BC_POP);
    bc()->addInsn(BC_POP);
    if (node->elseBlock()) {
        node->elseBlock()->visit(this);
    }

    bc()->bind(end);
}

void BytecodeGenerator::visitBlockNode(BlockNode* node) {
    Scope* blockScope = node->scope();
    BytecodeFunction* function = _functions.top();

    uint16_t scopeId = function->id();
    uint16_t varId = function->localsNumber();
    for (Scope::VarIterator it(blockScope); it.hasNext();) {
        AstVar* var = it.next();
        _var2scope[var] = scopeId;
        _var2id[var] = varId++;
    }
    function->setLocalsNumber(varId);

    for (Scope::FunctionIterator it(blockScope); it.hasNext();) {
        processFunction(it.next());
    }

    for (uint32_t i = 0; i < node->nodes(); ++i) {
        AstNode* childNode = node->nodeAt(i);
        childNode->visit(this);

        VarType resultType = _preprocessor->getType(childNode);
        if (resultType != VT_VOID && !childNode->isReturnNode()) {
            bc()->addInsn(BC_POP);
        }
        //bc()->addInsn(BC_BREAK);
    }
}

void BytecodeGenerator::processFunction(AstFunction* astFunction, bool topFunction) {
    if (topFunction) {
        preProcessFunctions(astFunction->scope());
    }

    BytecodeFunction* bcFunction = _code->functionByName(astFunction->name());
    uint16_t scopeId = bcFunction->id();
    bcFunction->setScopeId(scopeId);

    //assert(bcFunction->localsNumber() == 0);
    uint16_t varId = bcFunction->localsNumber();
    for (Scope::VarIterator it(astFunction->scope()); it.hasNext();) {
        AstVar* var = it.next();
        _var2scope[var] = scopeId;
        _var2id[var] = varId++;
    }
    bcFunction->setLocalsNumber(varId);

    _functions.push(bcFunction);
    visitFunctionNode(astFunction->node());
    _functions.pop();

    //bcFunction->bytecode()->addInsn(BC_RETURN);
    if (topFunction) {
        bcFunction->bytecode()->addInsn(BC_STOP);
    }
}

void BytecodeGenerator::preProcessFunctions(Scope* scope) {
    for (Scope::FunctionIterator it(scope); it.hasNext();) {
        _code->addFunction(new BytecodeFunction(it.next()));
    }

    for (uint32_t i = 0; i < scope->childScopeNumber(); ++i) {
        preProcessFunctions(scope->childScopeAt(i));
    }
}

void BytecodeGenerator::visitFunctionNode(FunctionNode* node) {
    node->body()->visit(this);
}

void BytecodeGenerator::visitReturnNode(ReturnNode* node) {
    if (node->returnExpr()) {
        node->returnExpr()->visit(this);

        VarType returnType = _preprocessor->getType(node);
        VarType exprType = _preprocessor->getType(node->returnExpr());
        cast(returnType, exprType, bc());
    }

    bc()->addInsn(BC_RETURN);
}

void BytecodeGenerator::visitCallNode(CallNode* node) {
    TranslatedFunction* function = _code->functionByName(node->name());
    for (uint32_t i = 0; i < node->parametersNumber(); ++i) {
        node->parameterAt(i)->visit(this);

        VarType argType = function->parameterType(i);
        VarType exprType = _preprocessor->getType(node->parameterAt(i));
        cast(argType, exprType, bc());
    }

    bc()->addInsn(BC_CALL);
    bc()->addUInt16(function->id());
}

void BytecodeGenerator::visitNativeCallNode(NativeCallNode* node) {
    void* compiledPtr = _code->processNativeFunction(&node->nativeSignature(), node->info());
    uint16_t id = _code->makeNativeFunction(node->nativeName(), node->nativeSignature(), compiledPtr);

    bc()->addInsn(BC_CALLNATIVE);
    bc()->addUInt16(id);
}

void BytecodeGenerator::visitPrintNode(PrintNode* node) {
    for (uint32_t i = 0; i < node->operands(); ++i) {
        node->operandAt(i)->visit(this);

        VarType type = _preprocessor->getType(node->operandAt(i));
        caseIntDoubleString(type, BC_IPRINT, BC_DPRINT, BC_SPRINT, bc());
    }
}
