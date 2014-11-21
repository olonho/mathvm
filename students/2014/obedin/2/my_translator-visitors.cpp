
#define PUSH_NODE m_sourcePos.push(node->position());
#define POP_NODE  m_sourcePos.pop();

void
TVisitor::visitBinaryOpNode(BinaryOpNode *node)
{
    PUSH_NODE

    node->right()->visit(this);
    node->left()->visit(this);

    TokenKind op = node->kind();
    switch (op) {
        case tOR: case tAND:
            genBooleanOp(op); break;
        case tAOR: case tAAND: case tAXOR:
            genBitwiseOp(op); break;
        case tEQ: case tNEQ: case tGT: case tGE: case tLT: case tLE:
            genComparisonOp(op); break;
        case tADD: case tSUB: case tMUL: case tDIV:
            genNumericOp(op); break;
        case tMOD:
            if (tosType() != VT_INT)
                throw std::runtime_error(MSG_NOT_INT_ON_TOS);
            bc()->addInsn(BC_IMOD);
            break;
        default:
            throw std::runtime_error(MSG_INVALID_BINARY);
    }

    POP_NODE
}

void
TVisitor::visitUnaryOpNode(UnaryOpNode *node)
{
    PUSH_NODE

    node->operand()->visit(this);
    switch (node->kind()) {
        case tNOT:
            booleanizeTos();
            bc()->addInsn(BC_ILOAD1);
            bc()->addInsn(BC_ISUB);
            break;
        case tSUB:
            bc()->addInsn(NUMERIC_INSN(tosType(), NEG)); break;
        default:
            throw std::runtime_error(MSG_INVALID_UNARY);
    }

    POP_NODE
}

void
TVisitor::visitStringLiteralNode(StringLiteralNode *node)
{
    if (node->literal().empty()) {
        bc()->addInsn(BC_SLOAD0);
    } else {
        bc()->addInsn(BC_SLOAD);
        bc()->addUInt16(m_code->makeStringConstant(node->literal()));
    }
    stackPush(VT_STRING);
}

void
TVisitor::visitDoubleLiteralNode(DoubleLiteralNode *node)
{
    bc()->addInsn(BC_DLOAD);
    bc()->addDouble(node->literal());
    stackPush(VT_DOUBLE);
}

void
TVisitor::visitIntLiteralNode(IntLiteralNode *node)
{
    bc()->addInsn(BC_ILOAD);
    bc()->addInt64(node->literal());
    stackPush(VT_INT);
}

void
TVisitor::visitLoadNode(LoadNode *node)
{
    PUSH_NODE

    loadVar(node->var());

    POP_NODE
}

void
TVisitor::visitStoreNode(StoreNode *node)
{
    PUSH_NODE

    node->value()->visit(this);

    switch (node->op()) {
        case tINCRSET:
            loadVar(node->var());
            castTosAndPrevToSameNumType();
            bc()->addInsn(NUMERIC_INSN(tosType(), ADD));
            stackPop();
            break;
        case tDECRSET:
            loadVar(node->var());
            castTosAndPrevToSameNumType();
            bc()->addInsn(NUMERIC_INSN(tosType(), SUB));
            stackPop();
            break;
        case tASSIGN:
            break;
        default:
            throw std::runtime_error(MSG_INVALID_STORE_OP);
    }
    storeVar(node->var());

    POP_NODE
}

void
TVisitor::visitForNode(ForNode *node)
{
    PUSH_NODE

    if (node->var()->type() != VT_INT)
        throw std::runtime_error(MSG_INVALID_FOR_VAR);

    if (!node->inExpr()->isBinaryOpNode())
        throw std::runtime_error(MSG_INVALID_FOR_EXPR);

    BinaryOpNode *inExpr = node->inExpr()->asBinaryOpNode();
    if (inExpr->kind() != tRANGE)
        throw std::runtime_error(MSG_INVALID_FOR_EXPR);

    const AstVar *itVar = node->var();
    Label lStart(bc()), lEnd(bc());

    inExpr->left()->visit(this);
    if (tosType() != VT_INT)
        throw std::runtime_error(MSG_INVALID_FOR_EXPR);
    storeVar(itVar);

    bc()->bind(lStart);
    inExpr->right()->visit(this);
    if (tosType() != VT_INT)
        throw std::runtime_error(MSG_INVALID_FOR_EXPR);

    loadVar(itVar);
    bc()->addBranch(BC_IFICMPG, lEnd);
    stackPop();
    stackPop();

    node->body()->visit(this);

    loadVar(itVar);
    bc()->addInsn(BC_ILOAD1);
    bc()->addInsn(BC_IADD);
    storeVar(itVar);
    bc()->addBranch(BC_JA, lStart);

    bc()->bind(lEnd);

    POP_NODE
}

void
TVisitor::visitWhileNode(WhileNode *node)
{
    PUSH_NODE

    Label lStart(bc()), lEnd(bc());
    bc()->bind(lStart);
    node->whileExpr()->visit(this);
    bc()->addInsn(BC_ILOAD0);
    bc()->addBranch(BC_IFICMPE, lEnd);
    node->loopBlock()->visit(this);
    bc()->addBranch(BC_JA, lStart);
    bc()->bind(lEnd);

    POP_NODE
}

void
TVisitor::visitIfNode(IfNode *node)
{
    PUSH_NODE

    Label lElse(bc()), lEnd(bc());
    node->ifExpr()->visit(this);
    bc()->addInsn(BC_ILOAD0);
    bc()->addBranch(BC_IFICMPE, lElse);
    node->thenBlock()->visit(this);
    if (node->elseBlock() != NULL)
        bc()->addBranch(BC_JA, lEnd);
    bc()->bind(lElse);
    if (node->elseBlock() != NULL)
        node->elseBlock()->visit(this);
    bc()->bind(lEnd);

    POP_NODE
}

void
TVisitor::visitBlockNode(BlockNode *node)
{
    PUSH_NODE

    initVars(node->scope());
    initFunctions(node->scope());
    genBlock(node);

    POP_NODE
}

void
TVisitor::visitFunctionNode(FunctionNode *node)
{
    PUSH_NODE

    BytecodeFunction *bcFn = (BytecodeFunction*) m_code->functionByName(node->name());
    if (bcFn == NULL)
        throw std::runtime_error("Can't be");

    bool isNative = node->body()->nodes() > 0
                 && node->body()->nodeAt(0)->isNativeCallNode();

    TScope scope(bcFn, m_curScope);
    m_curScope = &scope;
    std::vector<VarType> oldStack;
    m_stack.swap(oldStack);

    {
        for (size_t i = 0; i < node->parametersNumber(); ++i) {
            AstVar *var = node->body()->scope()->lookupVariable(node->parameterName(i));
            m_curScope->addVar(var);
            storeVar(var, false);
        }
        if (isNative)
            node->body()->nodeAt(0)->visit(this);
        else
            node->body()->visit(this);
    }

    m_stack.swap(oldStack);
    m_curScope = m_curScope->parent;

    POP_NODE
}

void
TVisitor::visitReturnNode(ReturnNode *node)
{
    PUSH_NODE

    if (node->returnExpr() != NULL) {
        node->returnExpr()->visit(this);
        castTos(m_curScope->fn->returnType());
    }
    bc()->addInsn(BC_RETURN);

    POP_NODE
}

void
TVisitor::visitCallNode(CallNode *node)
{
    PUSH_NODE

    BytecodeFunction *fn = (BytecodeFunction*) m_code->functionByName(node->name());
    if (fn == NULL)
        throw std::runtime_error(MSG_FUNCTION_NOT_FOUND);

    size_t paramsNum = node->parametersNumber();
    if (paramsNum != fn->parametersNumber())
        throw std::runtime_error(MSG_WRONG_NUM_OF_PARAMS);

    for (size_t i = paramsNum; paramsNum > 0 && i > 0; --i) {
        node->parameterAt(i-1)->visit(this);
        castTos(fn->parameterType(i-1));
    }

    bc()->addInsn(BC_CALL);
    bc()->addUInt16(fn->id());

    for (size_t i = 0; i < paramsNum; i++, stackPop());
    if (fn->returnType() != VT_VOID)
        stackPush(fn->returnType());

    POP_NODE
}

void
TVisitor::visitNativeCallNode(NativeCallNode *node)
{
    void *code = dlsym(RTLD_DEFAULT, node->nativeName().c_str());
    if (code == NULL)
        throw std::runtime_error(MSG_NATIVE_NOT_FOUND);
    uint16_t fnId = m_code->makeNativeFunction(node->nativeName(), node->nativeSignature(), code);
    bc()->addInsn(BC_CALLNATIVE);
    bc()->addUInt16(fnId);
    bc()->addInsn(BC_RETURN);
}

void
TVisitor::visitPrintNode(PrintNode *node)
{
    PUSH_NODE

    for (size_t i = 0; i < node->operands(); ++i) {
        node->operandAt(i)->visit(this);
        switch (tosType()) {
            case VT_INT:
                bc()->addInsn(BC_IPRINT); break;
            case VT_DOUBLE:
                bc()->addInsn(BC_DPRINT); break;
            case VT_STRING:
                bc()->addInsn(BC_SPRINT); break;
            default:
                throw std::runtime_error(MSG_UNPRINTABLE_TOS);
        }
        stackPop();
    }

    POP_NODE
}

#undef PUSH_NODE
#undef POP_NODE
