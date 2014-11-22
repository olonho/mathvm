
void
TVisitor::castTos(VarType to)
{
    if (tosType() == to)
        return;

    switch (to) {
        case VT_INT:
            if (tosType() == VT_DOUBLE) {
                bc()->addInsn(BC_D2I);
                break;
            }
        case VT_DOUBLE:
            if (tosType() == VT_INT) {
                bc()->addInsn(BC_I2D);
                break;
            }
        default:
            throw std::runtime_error(MSG_INVALID_CAST);
    }

    stackPop();
    stackPush(to);
}

void
TVisitor::booleanizeTos()
{
    castTos(VT_INT);
    Label lSetFalse(bc()), lEnd(bc());
    bc()->addInsn(BC_ILOAD0);
    bc()->addBranch(BC_IFICMPE, lSetFalse);
    bc()->addInsn(BC_ILOAD1);
    bc()->addBranch(BC_JA, lEnd);
    bc()->bind(lSetFalse);
    bc()->addInsn(BC_ILOAD0);
    bc()->bind(lEnd);
}

void
TVisitor::genBooleanOp(TokenKind op)
{
    castTosAndPrevToSameNumType();

    switch (op) {
        case tOR:
            bc()->addInsn(NUMERIC_INSN(tosType(), ADD)); break;
        case tAND:
            bc()->addInsn(NUMERIC_INSN(tosType(), MUL)); break;
        default: break;
    }

    stackPop();
    booleanizeTos();
}

void
TVisitor::genBitwiseOp(TokenKind op)
{
    // TODO: optimization -- don't cast and swap if they're already ints
    castTos(VT_INT);
    swapTos();
    castTos(VT_INT);
    swapTos();

    switch (op) {
        case tAOR:
            bc()->addInsn(BC_IAOR); break;
        case tAAND:
            bc()->addInsn(BC_IAAND); break;
        case tAXOR:
            bc()->addInsn(BC_IAXOR); break;
        default: break;
    }

    stackPop();
}

void
TVisitor::genComparisonOp(TokenKind op)
{
    castTosAndPrevToSameNumType();
    bc()->addInsn(NUMERIC_INSN(tosType(), CMP));

    switch (op) {
        case tEQ: // == 0
            bc()->addInsn(BC_ILOADM1);
            bc()->addInsn(BC_IAXOR);
            bc()->addInsn(BC_ILOAD1);
            bc()->addInsn(BC_IAAND);
            break;
        case tNEQ: // == (-1 || 1)
            bc()->addInsn(BC_ILOAD1);
            bc()->addInsn(BC_IAAND);
            break;
        case tGT: // == 1
            bc()->addInsn(BC_INEG);
        case tLT: // == -1
            bc()->addInsn(BC_ILOAD);
            bc()->addInt64(-2);
            bc()->addInsn(BC_IAAND);
            bc()->addInsn(BC_ILOAD);
            bc()->addInt64(2);
            bc()->addInsn(BC_IAAND);
            break;
        case tLE: // == (0 || -1)
            bc()->addInsn(BC_INEG);
        case tGE: // == (0 || 1)
            bc()->addInsn(BC_ILOAD);
            bc()->addInt64(2);
            bc()->addInsn(BC_IAXOR);
            bc()->addInsn(BC_ILOAD);
            bc()->addInt64(2);
            bc()->addInsn(BC_IAAND);
            break;
        default: break;
    }

    stackPop();
    stackPop();
    stackPush(VT_INT);
}

void
TVisitor::genNumericOp(TokenKind op)
{
    castTosAndPrevToSameNumType();
    switch (op) {
        case tADD:
            bc()->addInsn(NUMERIC_INSN(tosType(), ADD)); break;
        case tSUB:
            bc()->addInsn(NUMERIC_INSN(tosType(), SUB)); break;
        case tMUL:
            bc()->addInsn(NUMERIC_INSN(tosType(), MUL)); break;
        case tDIV:
            bc()->addInsn(NUMERIC_INSN(tosType(), DIV)); break;
        default: break;
    }
    stackPop();
}

void
TVisitor::castTosAndPrevToSameNumType()
{
    VarType tos  = tosType();
    VarType prev = m_stack.at(m_stack.size()-2);

    if (!IS_NUMERIC(tos) || !IS_NUMERIC(prev))
        throw std::runtime_error(MSG_NAN_ON_TOS_OR_PREV);

    if (tos == prev) {
        return;
    } else if (prev == VT_DOUBLE) {
        castTos(VT_DOUBLE);
    } else {
        swapTos();
        castTos(VT_DOUBLE);
        swapTos();
    }
}

void
TVisitor::loadVar(const AstVar *astVar)
{
    TVar var = m_curScope->findVar(astVar);
    VarType type = astVar->type();

    if (var.contextId == m_curScope->id()) {
        switch(var.id) {
            case 0: bc()->addInsn(LOAD_VAR(type, 0)); break;
            case 1: bc()->addInsn(LOAD_VAR(type, 1)); break;
            case 2: bc()->addInsn(LOAD_VAR(type, 2)); break;
            case 3: bc()->addInsn(LOAD_VAR(type, 3)); break;
            default:
                bc()->addInsn(LOAD_VAR(type, ));
                bc()->addUInt16(var.id);
                break;
        }
    } else {
        bc()->addInsn(LOAD_CTX_VAR(type));
        bc()->addUInt16(var.contextId);
        bc()->addUInt16(var.id);
    }

    stackPush(type);
}

void
TVisitor::storeVar(const AstVar *astVar, bool checkTos)
{
    TVar var = m_curScope->findVar(astVar);
    VarType type = astVar->type();
    if (checkTos)
        castTos(type);

    if (var.contextId == m_curScope->id()) {
        switch(var.id) {
            case 0: bc()->addInsn(STORE_VAR(type, 0)); break;
            case 1: bc()->addInsn(STORE_VAR(type, 1)); break;
            case 2: bc()->addInsn(STORE_VAR(type, 2)); break;
            case 3: bc()->addInsn(STORE_VAR(type, 3)); break;
            default:
                bc()->addInsn(STORE_VAR(type, ));
                bc()->addUInt16(var.id);
                break;
        }
    } else {
        bc()->addInsn(STORE_CTX_VAR(type));
        bc()->addUInt16(var.contextId);
        bc()->addUInt16(var.id);
    }

    if (checkTos)
        stackPop();
}

void
TVisitor::initVars(Scope *scope)
{
    Scope::VarIterator it(scope);
    while (it.hasNext())
        m_curScope->addVar(it.next());
}

void
TVisitor::initFunctions(Scope *scope)
{
    Scope::FunctionIterator it(scope);
    while (it.hasNext()) {
        AstFunction *fn = it.next();
        BytecodeFunction *bcFn = (BytecodeFunction*) m_code->functionByName(fn->name());
        if (bcFn == NULL) {
            bcFn = new BytecodeFunction(fn);
            m_code->addFunction(bcFn);
        }
    }

    it = Scope::FunctionIterator(scope);
    while (it.hasNext())
        it.next()->node()->visit(this);
}

void
TVisitor::genBlock(BlockNode *node)
{
    for (size_t i = 0; i < node->nodes(); ++i)
        node->nodeAt(i)->visit(this);
}

void
TScope::addVar(const AstVar *var)
{
    vars.insert(std::make_pair(var, vars.size()));
}

TVar
TScope::findVar(const AstVar *var)
{
    std::map<const AstVar *, Id>::iterator match =
        vars.find(var);
    if (match != vars.end())
        return TVar(match->second, id());
    else if (parent != NULL)
        return parent->findVar(var);
    else
        throw std::runtime_error(MSG_VAR_NOT_FOUND);
}
