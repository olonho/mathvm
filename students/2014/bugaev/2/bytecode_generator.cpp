#include "interpreter_code.hpp"
#include "bytecode_generator.hpp"

#include <dlfcn.h>


namespace mathvm
{

Status *BytecodeGenerator::generate(InterpreterCodeImpl *code,
                                    AstFunction *top)
{
    m_types.clear();
    m_code = code;

    Status *result = 0;
    try {
        registerFunction(top);
        translateFunction(top);
    } catch (BytecodeGeneratorException const &e) {
        result = Status::Error(e.what(), e.position());
    }

    m_code = 0;
    return result ? result : Status::Ok();
}


void BytecodeGenerator::visitBinaryOpNode(BinaryOpNode *node)
{
    if (node->kind() == tOR || node->kind() == tAND) {
        makeLazyLogicalOp(node);
        return;
    }

    node->right()->visit(this);
    node->left()->visit(this);

    VarType resultType = VT_INVALID;

    switch (node->kind()) {
    case tOR:
        assert(false);
        break;
    case tAND:
        assert(false);
        break;
    case tAOR:
        resultType = makeIBinaryOp(node, BC_IAOR);
        break;
    case tAAND:
        resultType = makeIBinaryOp(node, BC_IAAND);
        break;
    case tAXOR:
        resultType = makeIBinaryOp(node, BC_IAXOR);
        break;
    case tEQ:
        resultType = castIntDouble(node);
        bc()->addInsn((resultType == VT_INT) ? BC_ICMP : BC_DCMP);
        bc()->addInsn(BC_ILOAD0);
        bc()->addInsn(BC_ICMP);
        break;
    case tNEQ:
        resultType = castIntDouble(node);
        bc()->addInsn((resultType == VT_INT) ? BC_ICMP : BC_DCMP);
        break;
    case tGT:
        resultType = makeCmpOp(node, 2, false);
        break;
    case tGE:
        resultType = makeCmpOp(node, 3, false);
        break;
    case tLT:
        resultType = makeCmpOp(node, 2, true);
        break;
    case tLE:
        resultType = makeCmpOp(node, 3, true);
        break;
    case tADD:
        resultType = makeIDBinaryOp(node, BC_IADD, BC_DADD);
        break;
    case tSUB:
        resultType = makeIDBinaryOp(node, BC_ISUB, BC_DSUB);
        break;
    case tMUL:
        resultType = makeIDBinaryOp(node, BC_IMUL, BC_DMUL);
        break;
    case tDIV:
        resultType = makeIDBinaryOp(node, BC_IDIV, BC_DDIV);
        break;
    case tMOD:
        resultType = makeIBinaryOp(node, BC_IMOD);
        break;
    default:
        throw BytecodeGeneratorException("Illegal binary operator",
                                         node->position());
    }

    popType();
    popType();
    pushType(resultType);
}


void BytecodeGenerator::visitUnaryOpNode(UnaryOpNode *node)
{
    node->operand()->visit(this);
    switch (node->kind()) {
    case tSUB:
        if (tosType() != VT_DOUBLE && tosType() != VT_INT) {
            throw BytecodeGeneratorException("Invalid unary minus",
                                             node->position());
        }
        bc()->addInsn((tosType() == VT_INT) ? BC_INEG : BC_DNEG);
        break;
    case tNOT:
        if (tosType() != VT_INT)
            throw BytecodeGeneratorException("Invalid logical not",
                                             node->position());
        bc()->addInsn(BC_ILOAD0);
        bc()->addInsn(BC_ICMP);
        break;
    default:
        throw BytecodeGeneratorException("Illegal unary operator",
                                         node->position());
    }
}


void BytecodeGenerator::visitStringLiteralNode(StringLiteralNode *node)
{
    uint16_t const id = m_code->makeStringConstant(node->literal());
    bc()->addInsn(BC_SLOAD);
    bc()->addUInt16(id);
    pushType(VT_STRING);
}


void BytecodeGenerator::visitDoubleLiteralNode(DoubleLiteralNode *node)
{
    double const literal = node->literal();
    if (literal == 0.0) {
        bc()->addInsn(BC_DLOAD0);
    } else if (literal == 1.0) {
        bc()->addInsn(BC_DLOAD1);
    } else if (literal == -1.0) {
        bc()->addInsn(BC_DLOADM1);
    } else {
        bc()->addInsn(BC_DLOAD);
        bc()->addDouble(literal);
    }
    pushType(VT_DOUBLE);
}


void BytecodeGenerator::visitIntLiteralNode(IntLiteralNode *node)
{
    int64_t const literal = node->literal();
    switch (literal) {
    case 0:
        bc()->addInsn(BC_ILOAD0);
        break;
    case 1:
        bc()->addInsn(BC_ILOAD1);
        break;
    case -1:
        bc()->addInsn(BC_ILOADM1);
        break;
    default:
        bc()->addInsn(BC_ILOAD);
        bc()->addInt64(literal);
    }
    pushType(VT_INT);
}


void BytecodeGenerator::visitLoadNode(LoadNode *node)
{
    VarType type;
    uint16_t ctx;
    uint16_t id;
    findVariable(node, node->var()->name(), ctx, id, type);

    if (type != node->var()->type()) {
        throw BytecodeGeneratorException("Variable type mismatch",
                                         node->position());
    }

    loadVariable(node, type, ctx, id);
}


void BytecodeGenerator::visitStoreNode(StoreNode *node)
{
    VarType type;
    uint16_t ctx;
    uint16_t id;
    findVariable(node, node->var()->name(), ctx, id, type);

    node->value()->visit(this);

    switch (node->op()) {
    case tASSIGN:
        break;
    case tINCRSET:
        loadVariable(node, type, ctx, id);
        makeIDBinaryOp(node, BC_IADD, BC_DADD);
        break;
    case tDECRSET:
        loadVariable(node, type, ctx, id);
        makeIDBinaryOp(node, BC_ISUB, BC_DSUB);
        break;
    default:
        throw BytecodeGeneratorException("Invalid assignment",
                                         node->position());
    }

    storeVariable(node, type, ctx, id);
    loadVariable(node, type, ctx, id);
}


void BytecodeGenerator::visitForNode(ForNode *node)
{
    uint16_t ctx;
    uint16_t id;
    VarType type;
    findVariable(node, node->var()->name(), ctx, id, type);

    if (type != VT_INT || !node->inExpr()->isBinaryOpNode()
            || node->inExpr()->asBinaryOpNode()->kind() != tRANGE) {
        throw BytecodeGeneratorException("Invalid for", node->position());
    }

    node->inExpr()->asBinaryOpNode()->right()->visit(this);
    node->inExpr()->asBinaryOpNode()->left()->visit(this);
    storeVariable(node, type, ctx, id);
    storeVariable(node, type, ctx, 0);

    Label l2(bc()->currentLabel());

    loadVariable(node, type, ctx, 0);
    loadVariable(node, type, ctx, id);

    Label l1(bc());
    bc()->addBranch(BC_IFICMPG, l1);

    node->body()->visit(this);

    loadVariable(node, type, ctx, id);
    bc()->addInsn(BC_ILOAD1);
    bc()->addInsn(BC_IADD);
    storeVariable(node, type, ctx, id);

    bc()->addBranch(BC_JA, l2);
    bc()->bind(l1);
}


void BytecodeGenerator::visitWhileNode(WhileNode *node)
{
    Label l2(bc()->currentLabel());
    node->whileExpr()->visit(this);

    if (tosType() != VT_INT) {
        throw BytecodeGeneratorException("Invalid while",
                                         node->position());
    }

    Label l1(bc());
    bc()->addInsn(BC_ILOAD0);
    bc()->addBranch(BC_IFICMPE, l1);

    node->loopBlock()->visit(this);

    bc()->addBranch(BC_JA, l2);
    bc()->bind(l1);
}


void BytecodeGenerator::visitIfNode(IfNode *node)
{
    node->ifExpr()->visit(this);

    if (tosType() != VT_INT) {
        throw BytecodeGeneratorException("Invalid if",
                                         node->position());
    }

    Label l1(bc());
    bc()->addInsn(BC_ILOAD0);
    bc()->addBranch(BC_IFICMPE, l1);

    node->thenBlock()->visit(this);

    Label l2(bc());
    if (node->elseBlock())
        bc()->addBranch(BC_JA, l2);

    bc()->bind(l1);
    if (node->elseBlock()) {
        node->elseBlock()->visit(this);
        bc()->bind(l2);
    }
}


void BytecodeGenerator::visitBlockNode(BlockNode *node)
{
    if (node->scope() != scope())
        m_scopes.push_back(std::make_pair(static_cast<FunctionNode *>(0),
                                          node->scope()));

    registerFunctions(Scope::FunctionIterator(node->scope()));
    translateFunctions(Scope::FunctionIterator(node->scope()));

    for (uint32_t i = 0; i < node->nodes(); ++i) {
        node->nodeAt(i)->visit(this);

        if (hasNoReturn(node->nodeAt(i)))
            continue;
        bc()->addInsn(BC_POP);
        popType();
    }

    m_scopes.pop_back();
}


void BytecodeGenerator::visitFunctionNode(FunctionNode *node)
{
    bool native = node->body()->nodeAt(0)
            && node->body()->nodeAt(0)->isNativeCallNode();
    if (native)
        node->body()->nodeAt(0)->visit(this);
    else
        node->body()->visit(this);

    if (returnType() != VT_VOID && returnCount() == 0) {
        throw BytecodeGeneratorException("Function has no return",
                                         node->position());
    }

    bc()->bind(returnLabel());

    bool const stop = node->name() == AstFunction::top_name;
    bc()->addInsn(stop ? BC_STOP : BC_RETURN);
}


void BytecodeGenerator::visitReturnNode(ReturnNode *node)
{
    ++returnCount();

    if (node->returnExpr()) {
        node->returnExpr()->visit(this);
    } else if (returnType() == VT_VOID) {
        pushType(returnType());
        bc()->addBranch(BC_JA, returnLabel());
        return;
    } else {
        throw BytecodeGeneratorException("Empty return", node->position());
    }

    storeVariable(node, returnType(), currentContext(), 0,
                  "Invalid return type");
    pushType(returnType());

    bc()->addBranch(BC_JA, returnLabel());
}


void BytecodeGenerator::visitCallNode(CallNode *node)
{
    TranslatedFunction *tf = m_code->functionByName(node->name());
    if (!tf) {
        throw BytecodeGeneratorException("Undefined function call",
                                         node->position());
    }

    for (uint32_t i = 0; i < node->parametersNumber(); ++i) {
        node->parameterAt(i)->visit(this);
        storeVariable(node, tosType(), currentContext() + 1, i + 1);
    }
    bc()->addInsn(BC_CALL);
    bc()->addInt16(tf->id());

    loadVariable(node, tf->returnType(), currentContext() + 1, 0);
}


void BytecodeGenerator::visitNativeCallNode(NativeCallNode *node)
{
    void *code = dlsym(RTLD_DEFAULT, node->nativeName().c_str());
    if (!code) {
        throw BytecodeGeneratorException("Native function is not found",
                                         node->position());
    }

    uint16_t const id = m_code->makeNativeFunction(node->nativeName(),
                                                   node->nativeSignature(),
                                                   code);
    bc()->addInsn(BC_CALLNATIVE);
    bc()->addInt16(id);

    ++returnCount();
    pushType(returnType());

    bc()->addBranch(BC_JA, returnLabel());
}


void BytecodeGenerator::visitPrintNode(PrintNode *node)
{
    for (uint32_t i = 0; i < node->operands(); ++i) {
        node->operandAt(i)->visit(this);
        switch (tosType()) {
        case VT_INT:
            bc()->addInsn(BC_IPRINT);
            break;
        case VT_DOUBLE:
            bc()->addInsn(BC_DPRINT);
            break;
        case VT_STRING:
            bc()->addInsn(BC_SPRINT);
            break;
        default:
            throw BytecodeGeneratorException("Not printable type",
                                             node->position());
        }
        popType();
    }
}


void BytecodeGenerator::registerFunction(AstFunction *func)
{
    if (m_code->functionByName(func->name())) {
        throw BytecodeGeneratorException("Duplicate function declaration",
                                         func->node()->position());
    }
    BytecodeFunction *bfunc = new BytecodeFunction(func);
    m_code->addFunction(bfunc);
}


void BytecodeGenerator::translateFunction(AstFunction *top)
{
    BytecodeFunction *func = dynamic_cast<BytecodeFunction *>(
            m_code->functionByName(top->name()));
    assert(func);

    m_scopes.push_back(std::make_pair(top->node(),
                                      top->node()->body()->scope()));

    m_bcs.push_back(func->bytecode());
    m_returnLabels.push_back(Label(bc()));
    m_returnTypes.push_back(top->returnType());
    m_returnCounts.push_back(0);

    top->node()->visit(this);

    m_bcs.pop_back();
    m_returnLabels.pop_back();
    m_returnTypes.pop_back();
    m_returnCounts.pop_back();
}


void BytecodeGenerator::findVariable(
        AstNode const *node, std::string const &name,
        uint16_t &ctx, uint16_t &id, VarType &type)
{
    uint16_t curCtx = currentContext();
    while (true) {
        FunctionNode *fn = m_scopes[curCtx].first;
        Scope *scp = m_scopes[curCtx].second;

        if (fn) {
            for (uint32_t i = 0; i < fn->parametersNumber(); ++i) {
                if (fn->parameterName(i) == name) {
                    ctx = curCtx;
                    id = 1 + i;
                    type = fn->parameterType(i);
                    return;
                }
            }
        }

        int i = 0;
        Scope::VarIterator vi(scp);
        while (vi.hasNext()) {
            AstVar *av = vi.next();
            if (av->name() == name) {
                ctx = curCtx;
                id = 1 + i + fn->parametersNumber();
                type = av->type();
                return;
            }
            ++i;
        }

        if (curCtx == 0)
            break;
        --curCtx;
    }

    throw BytecodeGeneratorException("Undefined variable",
                                     node->position());
}


void BytecodeGenerator::loadVariable(AstNode const *node,
                                     VarType type, uint16_t ctx, uint16_t id)
{
    switch (type) {
    case VT_INT:
        bc()->addInsn(BC_LOADCTXIVAR);
        break;
    case VT_DOUBLE:
        bc()->addInsn(BC_LOADCTXDVAR);
        break;
    case VT_STRING:
        bc()->addInsn(BC_LOADCTXSVAR);
        break;
    case VT_VOID:
        bc()->addInsn(BC_ILOAD0);
        break;
    default:
        throw BytecodeGeneratorException("Couldn't load variable",
                                         node->position());
    }

    if (type != VT_VOID) {
        bc()->addInt16(ctx);
        bc()->addInt16(id);
    }

    pushType(type);
}


void BytecodeGenerator::storeVariable(
        AstNode const *node,
        VarType type, uint16_t ctx, uint16_t id,
        char const *errMsg)
{
    if (errMsg == 0)
        errMsg = "Couldn't store variable";

    castTypes(node, tosType(), type);

    switch (type) {
    case VT_INT:
        bc()->addInsn(BC_STORECTXIVAR);
        break;
    case VT_DOUBLE:
        bc()->addInsn(BC_STORECTXDVAR);
        break;
    case VT_STRING:
        bc()->addInsn(BC_STORECTXSVAR);
        break;
    default:
        throw BytecodeGeneratorException(errMsg, node->position());
    }

    bc()->addInt16(ctx);
    bc()->addInt16(id);

    popType();
}


void BytecodeGenerator::makeLazyLogicalOp(BinaryOpNode *node)
{
    assert(node->kind() == tOR || node->kind() == tAND);
    bool const andOp = node->kind() == tAND;

    node->left()->visit(this);
    if (tosType() != VT_INT) {
        throw BytecodeGeneratorException("Illegal operands",
                                         node->position());
    }
    popType();

    bc()->addInsn(BC_ILOAD0);
    Label l1(bc());
    bc()->addBranch(andOp ? BC_IFICMPNE : BC_IFICMPE, l1);

    bc()->addInsn(andOp ? BC_ILOAD0 : BC_ILOAD1);
    Label l2(bc());
    bc()->addBranch(BC_JA, l2);

    bc()->bind(l1);
    node->right()->visit(this);
    if (tosType() != VT_INT) {
        throw BytecodeGeneratorException("Illegal operands",
                                         node->position());
    }
    popType();

    bc()->bind(l2);
    pushType(VT_INT);
}


VarType BytecodeGenerator::makeIBinaryOp(AstNode const *node, Instruction insn)
{
    if (tosType() != VT_INT || ptosType() != VT_INT)
        throw BytecodeGeneratorException("Illegal operands",
                                         node->position());
    bc()->addInsn(insn);
    return VT_INT;
}


VarType BytecodeGenerator::makeIDBinaryOp(
        AstNode const *node,
        Instruction ibc, Instruction dbc)
{
    VarType result = castIntDouble(node);
    bc()->addInsn((result == VT_INT) ? ibc : dbc);
    return result;
}


VarType BytecodeGenerator::makeCmpOp(
        AstNode const *node, int64_t mask, bool swap)
{
    VarType result = castIntDouble(node);
    if (swap) bc()->addInsn(BC_SWAP);
    bc()->addInsn((result == VT_INT) ? BC_ICMP : BC_DCMP);
    bc()->addInsn(BC_ILOAD1);
    bc()->addInsn(BC_IADD);
    bc()->addInsn(BC_ILOAD);
    bc()->addInt64(mask);
    bc()->addInsn(BC_IAAND);
    return result;
}


VarType BytecodeGenerator::castIntDouble(AstNode const *node)
{
    if ((tosType() != VT_INT && tosType() != VT_DOUBLE)
            || (ptosType() != VT_INT && ptosType() != VT_DOUBLE)) {
        throw BytecodeGeneratorException("Illegal operands",
                                         node->position());
    }
    bool tosd = tosType() == VT_DOUBLE;
    bool ptosd = ptosType() == VT_DOUBLE;
    if (tosd && !ptosd) {
        bc()->addInsn(BC_SWAP);
        bc()->addInsn(BC_I2D);
        bc()->addInsn(BC_SWAP);
    } else if (ptosd && !tosd) {
        bc()->addInsn(BC_I2D);
    }
    return (tosd || ptosd) ? VT_DOUBLE : VT_INT;
}


void BytecodeGenerator::castTypes(AstNode const *node,
                                  VarType from, VarType to)
{
    if (from == to)
        return;
    if (from == VT_INT && to == VT_DOUBLE) {
        bc()->addInsn(BC_I2D);
    } else if (from == VT_DOUBLE && to == VT_INT) {
        bc()->addInsn(BC_D2I);
    } else {
        throw BytecodeGeneratorException("Illegal type cast",
                                         node->position());
    }
}

}
